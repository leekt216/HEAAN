#include "Scheme.h"

#include <NTL/ZZ.h>
#include <cmath>
#include <iostream>

#include "../czz/CZZ.h"
#include "../czz/CZZX.h"
#include "../utils/NumUtils.h"
#include "../utils/Ring2Utils.h"

using namespace std;
using namespace NTL;

void Scheme::rlweInstance(CZZX& c0, CZZX& c1) {
	CZZX v, e;
	NumUtils::sampleZO(v, params.n, params.rho);
	Ring2Utils::multNew(c0, v, publicKey.b, params.logq, params.q, params.n);
	NumUtils::sampleGauss(e, params.n, params.sigma);
	Ring2Utils::addNew(c0, e, c0, params.q, params.n);

	Ring2Utils::multNew(c1, v, publicKey.a, params.logq, params.q, params.n);
	NumUtils::sampleGauss(e, params.n, params.sigma);
	Ring2Utils::addNew(c1, e, c1, params.q, params.n);
}

void Scheme::trueValue(CZZ& m, ZZ& qi) {
	while(2 * m.r > qi) m.r -= qi;
	while(2 * m.r < -qi) m.r += qi;

	while(2 * m.i > qi) m.i -= qi;
	while(2 * m.i < -qi) m.i += qi;
}

Cipher Scheme::encrypt(CZZ& m, ZZ& nu) {
	CZZX c0, c1;
	rlweInstance(c0, c1);

	CZZ tmp = coeff(c0, 0) + m;
	Ring2Utils::truncate(tmp, params.logq);
	SetCoeff(c0, 0, tmp);
//	Cipher cipher(c0, c1, 1, params.Bclean, nu);
	Cipher cipher(c0, c1, 1);
	return cipher;
}


CZZ Scheme::decrypt(Cipher& cipher) {
	long logqi = getLogqi(cipher.level);
	ZZ qi = getqi(cipher.level);
	CZZX poly;
	CZZ m;

	Ring2Utils::multNew(poly, cipher.c1, secretKey.s, logqi, qi, params.n);
	Ring2Utils::addNew(poly, poly, cipher.c0, qi, params.n);
	GetCoeff(m, poly, 0);
	trueValue(m, qi);
	return m;
}

void Scheme::decrypt(vector<CZZ>& res, vector<Cipher>& ciphers) {
	res.clear();
	for (long i = 0; i < ciphers.size(); ++i) {
		CZZ d = decrypt(ciphers[i]);
		res.push_back(d);
	}
}

CZZX Scheme::encode(long& logSlots, vector<CZZ>& mvec) {
	CZZX res;
	vector<CZZ> fftInv = NumUtils::fftInv(mvec, params.cksi);

	long idx = 0;
	long slots = 1 << logSlots;
	long gap = (params.n >> logSlots);

	for (long i = 0; i < slots; ++i) {
		SetCoeff(res, idx, fftInv[i]);
		idx += gap;
	}

	return res;
}

Cipher Scheme::encrypt(long& logSlots, vector<CZZ>& mvec, ZZ& nu) {
	CZZX c0, c1;
	rlweInstance(c0, c1);

	CZZX f = encode(logSlots, mvec);
	Ring2Utils::addNew(c0, f, c0, params.q, params.n);

//	Cipher cipher(c0, c1, 1, params.Bclean, nu);
	Cipher cipher(c0, c1, 1);
	return cipher;
}

vector<CZZ> Scheme::decrypt(long& logSlots, Cipher& cipher) {
	long logqi = getLogqi(cipher.level);
	ZZ qi = getqi(cipher.level);
	long idx = 0;
	CZZX poly;
	CZZ c;
	vector<CZZ> fft;

	long slots = (1 << logSlots);
	long gap = (params.n >> logSlots);

	Ring2Utils::multNew(poly, cipher.c1, secretKey.s, logqi, qi, params.n);
	Ring2Utils::addNew(poly, poly, cipher.c0, qi, params.n);

	for (long i = 0; i < slots; ++i) {
		c = coeff(poly, idx);
		trueValue(c, qi);
		fft.push_back(c);
		idx += gap;
	}

	vector<CZZ> res = NumUtils::fft(fft, params.cksi);
	return res;
}

CZZX Scheme::encodeAll(vector<CZZ>& mvec) {
	CZZX res;
	vector<CZZ> fftInv = NumUtils::fftInv(mvec, params.cksi);
	for (long i = 0; i < mvec.size(); ++i) {
		SetCoeff(res, i, fftInv[i]);
	}
	return res;
}

Cipher Scheme::encryptAll(vector<CZZ>& mvec, ZZ& nu) {
	CZZX c0, c1;
	rlweInstance(c0, c1);

	CZZX f = encodeAll(mvec);
	Ring2Utils::add(c0, f, c0, params.logq, params.n);

//	Cipher cipher(c0, c1, 1, params.Bclean, nu);
	Cipher cipher(c0, c1, 1);
	return cipher;

}

vector<CZZ> Scheme::decryptAll(Cipher& cipher) {
	long logqi = getLogqi(cipher.level);
	ZZ qi = getqi(cipher.level);
	vector<CZZ> fft;
	CZZX poly;
	CZZ c;
	Ring2Utils::multNew(poly, cipher.c1, secretKey.s, logqi, qi, params.n);
	Ring2Utils::addNew(poly, poly, cipher.c0, qi, params.n);
	for (long i = 0; i < params.n; ++i) {
		c = coeff(poly, i);
		trueValue(c, qi);

		fft.push_back(c);
	}

	vector<CZZ> res = NumUtils::fft(fft, params.cksi);
	return res;
}

//-----------------------------

Cipher Scheme::add(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	CZZX c0, c1;

	Ring2Utils::add(c0, cipher1.c0, cipher2.c0, logqi, params.n);
	Ring2Utils::add(c1, cipher1.c1, cipher2.c1, logqi, params.n);

//	ZZ eBnd = cipher1.eBnd + cipher2.eBnd;
//	ZZ mBnd = cipher1.mBnd + cipher2.mBnd;

//	Cipher res(c0, c1, cipher1.level, eBnd, mBnd);
	Cipher res(c0, c1, cipher1.level);
	return res;
}

void Scheme::addAndEqual(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	Ring2Utils::addAndEqual(cipher1.c0, cipher2.c0, logqi, params.n);
	Ring2Utils::addAndEqual(cipher1.c1, cipher2.c1, logqi, params.n);

//	cipher1.eBnd += cipher2.eBnd;
//	cipher1.mBnd += cipher2.mBnd;
}

Cipher Scheme::addNew(Cipher& cipher1, Cipher& cipher2) {
	ZZ qi = getqi(cipher1.level);
	CZZX c0, c1;

	Ring2Utils::addNew(c0, cipher1.c0, cipher2.c0, qi, params.n);
	Ring2Utils::addNew(c1, cipher1.c1, cipher2.c1, qi, params.n);

//	ZZ eBnd = cipher1.eBnd + cipher2.eBnd;
//	ZZ mBnd = cipher1.mBnd + cipher2.mBnd;

//	Cipher res(c0, c1, cipher1.level, eBnd, mBnd);
	Cipher res(c0, c1, cipher1.level);
	return res;
}

void Scheme::addAndEqualNew(Cipher& cipher1, Cipher& cipher2) {
	ZZ qi = getqi(cipher1.level);
	Ring2Utils::addAndEqualNew(cipher1.c0, cipher2.c0, qi, params.n);
	Ring2Utils::addAndEqualNew(cipher1.c1, cipher2.c1, qi, params.n);

//	cipher1.eBnd += cipher2.eBnd;
//	cipher1.mBnd += cipher2.mBnd;
}

Cipher Scheme::addConst(Cipher& cipher, ZZ& cnst) {
	long logqi = getLogqi(cipher.level);
	CZZX c0 = cipher.c0;
	CZZX c1 = cipher.c1;
	CZZ tmp = coeff(cipher.c0,0) + cnst;
	Ring2Utils::truncate(tmp, logqi);
	SetCoeff(c0, 0, tmp);
	c0.normalize();

//	ZZ eBnd = cipher.eBnd + 1;
//	ZZ mBnd = cipher.mBnd + cnst;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

Cipher Scheme::addConst(Cipher& cipher, CZZ& cnst) {
	long logqi = getLogqi(cipher.level);
	CZZX c0 = cipher.c0;
	CZZX c1 = cipher.c1;
	CZZ tmp = coeff(cipher.c0,0) + cnst;
	Ring2Utils::truncate(tmp, logqi);
	SetCoeff(c0, 0, tmp);
	c0.normalize();

//	ZZ norm = cnst.norm();

//	ZZ eBnd = cipher.eBnd + 1;
//	ZZ mBnd = cipher.mBnd + norm;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

void Scheme::addConstAndEqual(Cipher& cipher, ZZ& cnst) {
	long logqi = getLogqi(cipher.level);
	CZZ tmp = coeff(cipher.c0, 0) * cnst;
	Ring2Utils::truncate(tmp, logqi);
	SetCoeff(cipher.c0, 0, tmp);
	cipher.c0.normalize();
}

Cipher Scheme::addConstNew(Cipher& cipher, ZZ& cnst) {
	ZZ qi = getqi(cipher.level);
	CZZX c0 = cipher.c0;
	CZZX c1 = cipher.c1;

	AddMod(c0.rx.rep[0], cipher.c0.rx.rep[0], cnst, qi);
	c0.normalize();

//	ZZ eBnd = cipher.eBnd + 1;
//	ZZ mBnd = cipher.mBnd + cnst;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

Cipher Scheme::addConstNew(Cipher& cipher, CZZ& cnst) {
	ZZ qi = getqi(cipher.level);
	CZZX c0 = cipher.c0;
	CZZX c1 = cipher.c1;

	AddMod(c0.rx.rep[0], cipher.c0.rx.rep[0], cnst.r, qi);
	AddMod(c0.ix.rep[0], cipher.c0.ix.rep[0], cnst.i, qi);
	c0.normalize();

//	ZZ norm = cnst.norm();

//	ZZ eBnd = cipher.eBnd + 1;
//	ZZ mBnd = cipher.mBnd + norm;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

void Scheme::addConstAndEqualNew(Cipher& cipher, ZZ& cnst) {
	ZZ qi = getqi(cipher.level);
	AddMod(cipher.c0.rx.rep[0], cipher.c0.rx.rep[0], cnst, qi);
	cipher.c0.normalize();
}

Cipher Scheme::sub(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	CZZX c0 , c1;

	Ring2Utils::sub(c0, cipher1.c0, cipher2.c0, logqi, params.n);
	Ring2Utils::sub(c1, cipher1.c1, cipher2.c1, logqi, params.n);

//	ZZ B = cipher1.eBnd + cipher2.eBnd;
//	ZZ nu = cipher1.mBnd + cipher2.mBnd;

//	Cipher res(c0, c1, cipher1.level, B, nu);
	Cipher res(c0, c1, cipher1.level);
	return res;
}

void Scheme::subAndEqual(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	Ring2Utils::subAndEqual(cipher1.c0, cipher2.c0, logqi, params.n);
	Ring2Utils::subAndEqual(cipher1.c1, cipher2.c1, logqi, params.n);

//	cipher1.eBnd += cipher2.eBnd;
//	cipher1.mBnd += cipher2.mBnd;
}

Cipher Scheme::subNew(Cipher& cipher1, Cipher& cipher2) {
	ZZ qi = getqi(cipher1.level);
	CZZX c0 , c1;

	Ring2Utils::subNew(c0, cipher1.c0, cipher2.c0, qi, params.n);
	Ring2Utils::subNew(c1, cipher1.c1, cipher2.c1, qi, params.n);

//	ZZ B = cipher1.eBnd + cipher2.eBnd;
//	ZZ nu = cipher1.mBnd + cipher2.mBnd;

//	Cipher res(c0, c1, cipher1.level, B, nu);
	Cipher res(c0, c1, cipher1.level);
	return res;
}

void Scheme::subAndEqualNew(Cipher& cipher1, Cipher& cipher2) {
	ZZ qi = getqi(cipher1.level);
	Ring2Utils::subAndEqualNew(cipher1.c0, cipher2.c0, qi, params.n);
	Ring2Utils::subAndEqualNew(cipher1.c1, cipher2.c1, qi, params.n);

//	cipher1.eBnd += cipher2.eBnd;
//	cipher1.mBnd += cipher2.mBnd;
}

Cipher Scheme::mult(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	long logPqi = params.logP + logqi;

	CZZX cc00, cc01, cc10, cc11, mulC0, mulC1;

	Ring2Utils::mult(cc00, cipher1.c0, cipher2.c0, logqi, params.n);
	Ring2Utils::mult(cc01, cipher1.c0, cipher2.c1, logqi, params.n);
	Ring2Utils::mult(cc10, cipher1.c1, cipher2.c0, logqi, params.n);
	Ring2Utils::mult(cc11, cipher1.c1, cipher2.c1, logqi, params.n);

	Ring2Utils::mult(mulC1, cc11, publicKey.aStar, logPqi, params.n);
	Ring2Utils::mult(mulC0, cc11, publicKey.bStar, logPqi, params.n);

	Ring2Utils::rightShiftAndEqual(mulC1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(mulC0, params.logP, params.n);

	Ring2Utils::addAndEqual(mulC1, cc10, logqi, params.n);
	Ring2Utils::addAndEqual(mulC1, cc01, logqi, params.n);
	Ring2Utils::addAndEqual(mulC0, cc00, logqi, params.n);

//	ZZ eBnd = cipher1.eBnd * (cipher2.mBnd + cipher2.eBnd) + cipher1.mBnd * cipher2.eBnd;
//	ZZ mBnd = cipher1.mBnd * cipher2.mBnd;

//	Cipher cipher(mulC0, mulC1, cipher1.level, eBnd, mBnd);
	Cipher cipher(mulC0, mulC1, cipher1.level);
	return cipher;
}

Cipher Scheme::multNew(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	long logPqi = params.logP + logqi;
	ZZ qi = getqi(cipher1.level);
	ZZ Pqi = getPqi(cipher1.level);

	CZZX cc00, cc01, cc10, cc11, mulC0, mulC1;

	Ring2Utils::multNew(cc00, cipher1.c0, cipher2.c0, logqi, qi, params.n);
	Ring2Utils::multNew(cc01, cipher1.c0, cipher2.c1, logqi, qi, params.n);
	Ring2Utils::multNew(cc10, cipher1.c1, cipher2.c0, logqi, qi, params.n);
	Ring2Utils::multNew(cc11, cipher1.c1, cipher2.c1, logqi, qi, params.n);

	Ring2Utils::multNew(mulC1, cc11, publicKey.aStar, logPqi, Pqi, params.n);
	Ring2Utils::multNew(mulC0, cc11, publicKey.bStar, logPqi, Pqi, params.n);

	Ring2Utils::rightShiftAndEqual(mulC1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(mulC0, params.logP, params.n);

	Ring2Utils::addAndEqualNew(mulC1, cc10, qi, params.n);
	Ring2Utils::addAndEqualNew(mulC1, cc01, qi, params.n);
	Ring2Utils::addAndEqualNew(mulC0, cc00, qi, params.n);

//	ZZ eBnd = cipher1.eBnd * (cipher2.mBnd + cipher2.eBnd) + cipher1.mBnd * cipher2.eBnd;
//	ZZ mBnd = cipher1.mBnd * cipher2.mBnd;

//	Cipher cipher(mulC0, mulC1, cipher1.level, eBnd, mBnd);
	Cipher cipher(mulC0, mulC1, cipher1.level);
	return cipher;
}

void Scheme::multAndEqual(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	long logPqi = params.logP + logqi;

	CZZX cc00, cc01, cc10, cc11, mulC0, mulC1;

	Ring2Utils::mult(cc00, cipher1.c0, cipher2.c0, logqi, params.n);
	Ring2Utils::mult(cc01, cipher1.c0, cipher2.c1, logqi, params.n);
	Ring2Utils::mult(cc10, cipher1.c1, cipher2.c0, logqi, params.n);
	Ring2Utils::mult(cc11, cipher1.c1, cipher2.c1, logqi, params.n);

	Ring2Utils::mult(mulC1, cc11, publicKey.aStar, logPqi, params.n);
	Ring2Utils::mult(mulC0, cc11, publicKey.bStar, logPqi, params.n);

	Ring2Utils::rightShiftAndEqual(mulC1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(mulC0, params.logP, params.n);

	Ring2Utils::addAndEqual(mulC1, cc10, logqi, params.n);
	Ring2Utils::addAndEqual(mulC1, cc01, logqi, params.n);
	Ring2Utils::addAndEqual(mulC0, cc00, logqi, params.n);

	cipher1.c0 = mulC0;
	cipher1.c1 = mulC1;

//	cipher1.eBnd = cipher1.eBnd * (cipher2.mBnd + cipher2.eBnd) + cipher1.mBnd * cipher2.eBnd;
//	cipher1.mBnd = cipher1.mBnd * cipher2.mBnd;
}

void Scheme::multAndEqualNew(Cipher& cipher1, Cipher& cipher2) {
	long logqi = getLogqi(cipher1.level);
	long logPqi = params.logP + logqi;
	ZZ qi = getqi(cipher1.level);
	ZZ Pqi = getPqi(cipher1.level);

	CZZX cc00, cc01, cc10;

	Ring2Utils::multNew(cc00, cipher1.c0, cipher2.c0, logqi, qi, params.n);
	Ring2Utils::multNew(cc01, cipher1.c0, cipher2.c1, logqi, qi, params.n);
	Ring2Utils::multNew(cc10, cipher1.c1, cipher2.c0, logqi, qi, params.n);
	Ring2Utils::multNew(cipher1.c0, cipher1.c1, cipher2.c1, logqi, qi, params.n);

	Ring2Utils::multNew(cipher1.c1, cipher1.c0, publicKey.aStar, logPqi, Pqi, params.n);
	Ring2Utils::multAndEqualNew(cipher1.c0, publicKey.bStar, logPqi, Pqi, params.n);

	Ring2Utils::rightShiftAndEqual(cipher1.c1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(cipher1.c0, params.logP, params.n);

	Ring2Utils::addAndEqualNew(cipher1.c1, cc10, qi, params.n);
	Ring2Utils::addAndEqualNew(cipher1.c1, cc01, qi, params.n);
	Ring2Utils::addAndEqualNew(cipher1.c0, cc00, qi, params.n);

//	cipher1.eBnd = cipher1.eBnd * (cipher2.mBnd + cipher2.eBnd) + cipher1.mBnd * cipher2.eBnd;
//	cipher1.mBnd = cipher1.mBnd * cipher2.mBnd;
}

Cipher Scheme::square(Cipher& cipher) {
	long logqi = getLogqi(cipher.level);
	long logPqi = params.logP + logqi;

	CZZX cc00, cc10, cc11, mulC0, mulC1;

	Ring2Utils::square(cc00, cipher.c0, logqi, params.n);
	Ring2Utils::mult(cc10, cipher.c1, cipher.c0, logqi, params.n);
	Ring2Utils::leftShiftAndEqual(cc10, 1, logqi, params.n);
	Ring2Utils::square(cc11, cipher.c1, logqi, params.n);

	Ring2Utils::mult(mulC1, cc11, publicKey.aStar, logPqi, params.n);
	Ring2Utils::mult(mulC0, cc11, publicKey.bStar, logPqi, params.n);

	Ring2Utils::rightShiftAndEqual(mulC1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(mulC0, params.logP, params.n);

	Ring2Utils::addAndEqual(mulC1, cc10, logqi, params.n);
	Ring2Utils::addAndEqual(mulC0, cc00, logqi, params.n);

//	ZZ eBnd = 2 * cipher.eBnd * cipher.mBnd + cipher.eBnd * cipher.eBnd;
//	ZZ mBnd = cipher.mBnd * cipher.mBnd;

//	Cipher c(mulC0, mulC1, cipher.level, eBnd, mBnd);
	Cipher c(mulC0, mulC1, cipher.level);
	return c;
}

void Scheme::squareAndEqual(Cipher& cipher) {
	long logqi = getLogqi(cipher.level);
	long logPqi = params.logP + logqi;

	CZZX cc00, cc10, cc11, mulC0, mulC1;

	Ring2Utils::square(cc00, cipher.c0, logqi, params.n);
	Ring2Utils::mult(cc10, cipher.c0, cipher.c1, logqi, params.n);
	Ring2Utils::leftShiftAndEqual(cc10, 1, logqi, params.n);
	Ring2Utils::square(cc11, cipher.c1, logqi, params.n);

	Ring2Utils::mult(mulC1, cc11, publicKey.aStar, logPqi, params.n);
	Ring2Utils::mult(mulC0, cc11, publicKey.bStar, logPqi, params.n);

	Ring2Utils::rightShiftAndEqual(mulC1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(mulC0, params.logP, params.n);

	Ring2Utils::addAndEqual(mulC1, cc10, logqi, params.n);
	Ring2Utils::addAndEqual(mulC0, cc00, logqi, params.n);

	cipher.c0 = mulC0;
	cipher.c1 = mulC1;

//	cipher.eBnd *= 2 * cipher.mBnd + cipher.eBnd;
//	cipher.mBnd *= cipher.mBnd;
}

Cipher Scheme::squareNew(Cipher& cipher) {
	long logqi = getLogqi(cipher.level);
	long logPqi = params.logP + logqi;
	ZZ qi = getqi(cipher.level);
	ZZ Pqi = getPqi(cipher.level);

	CZZX cc00, cc10, cc11, mulC0, mulC1;

	Ring2Utils::squareNew(cc00, cipher.c0, logqi, qi, params.n);
	Ring2Utils::multNew(cc10, cipher.c1, cipher.c0, logqi, qi, params.n);
	Ring2Utils::addAndEqualNew(cc10, cc10, qi, params.n);
	Ring2Utils::squareNew(cc11, cipher.c1, logqi, qi, params.n);

	Ring2Utils::multNew(mulC1, cc11, publicKey.aStar, logPqi, Pqi, params.n);
	Ring2Utils::multNew(mulC0, cc11, publicKey.bStar, logPqi, Pqi, params.n);

	Ring2Utils::rightShiftAndEqual(mulC1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(mulC0, params.logP, params.n);

	Ring2Utils::addAndEqualNew(mulC1, cc10, qi, params.n);
	Ring2Utils::addAndEqualNew(mulC0, cc00, qi, params.n);

//	ZZ eBnd = 2 * cipher.eBnd * cipher.mBnd + cipher.eBnd * cipher.eBnd;
//	ZZ mBnd = cipher.mBnd * cipher.mBnd;

//	Cipher c(mulC0, mulC1, cipher.level, eBnd, mBnd);
	Cipher c(mulC0, mulC1, cipher.level);
	return c;
}

void Scheme::squareAndEqualNew(Cipher& cipher) {
	long logqi = getLogqi(cipher.level);
	long logPqi = params.logP + logqi;
	ZZ qi = getqi(cipher.level);
	ZZ Pqi = getPqi(cipher.level);

	CZZX cc00, cc10, cc11, mulC0, mulC1;

	Ring2Utils::squareNew(cc00, cipher.c0, logqi, qi, params.n);
	Ring2Utils::multNew(cc10, cipher.c0, cipher.c1, logqi, qi, params.n);
	Ring2Utils::addAndEqualNew(cc10, cc10, qi, params.n);
	Ring2Utils::squareNew(cc11, cipher.c1, logqi, qi, params.n);

	Ring2Utils::multNew(mulC1, cc11, publicKey.aStar, logPqi, Pqi, params.n);
	Ring2Utils::multNew(mulC0, cc11, publicKey.bStar, logPqi, Pqi, params.n);

	Ring2Utils::rightShiftAndEqual(mulC1, params.logP, params.n);
	Ring2Utils::rightShiftAndEqual(mulC0, params.logP, params.n);

	Ring2Utils::addAndEqualNew(mulC1, cc10, qi, params.n);
	Ring2Utils::addAndEqualNew(mulC0, cc00, qi, params.n);

	cipher.c0 = mulC0;
	cipher.c1 = mulC1;

//	cipher.eBnd *= 2 * cipher.mBnd + cipher.eBnd;
//	cipher.mBnd *= cipher.mBnd;
}

Cipher Scheme::multByConst(Cipher& cipher, ZZ& cnst) {
	long logqi = getLogqi(cipher.level);
	CZZX c0, c1;
	Ring2Utils::multByConst(c0, cipher.c0, cnst, logqi, params.n);
	Ring2Utils::multByConst(c1, cipher.c1, cnst, logqi, params.n);

//	ZZ eBnd = cipher.eBnd * cnst;
//	ZZ mBnd = cipher.mBnd * cnst;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

Cipher Scheme::multByConst(Cipher& cipher, CZZ& cnst) {
	long logqi = getLogqi(cipher.level);
	CZZX c0, c1;
	Ring2Utils::multByConst(c0, cipher.c0, cnst, logqi, params.n);
	Ring2Utils::multByConst(c1, cipher.c1, cnst, logqi, params.n);
//	ZZ norm = cnst.norm();

//	ZZ eBnd = cipher.eBnd * norm;
//	ZZ mBnd = cipher.mBnd * norm;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

void Scheme::multByConstAndEqual(Cipher& cipher, ZZ& cnst) {
	long logqi = getLogqi(cipher.level);
	Ring2Utils::multByConstAndEqual(cipher.c0, cnst, logqi, params.n);
	Ring2Utils::multByConstAndEqual(cipher.c1, cnst, logqi, params.n);

//	cipher.eBnd *= cnst;
//	cipher.mBnd *= cnst;
}

void Scheme::multByConstAndEqual(Cipher& cipher, CZZ& cnst) {
	long logqi = getLogqi(cipher.level);
	Ring2Utils::multByConstAndEqual(cipher.c0, cnst, logqi, params.n);
	Ring2Utils::multByConstAndEqual(cipher.c1, cnst, logqi, params.n);

//	ZZ norm = cnst.norm();

//	cipher.eBnd *= norm;
//	cipher.mBnd *= norm;
}


Cipher Scheme::multByConstNew(Cipher& cipher, ZZ& cnst) {
	ZZ qi = getqi(cipher.level);
	CZZX c0, c1;
	Ring2Utils::multByConstNew(c0, cipher.c0, cnst, qi, params.n);
	Ring2Utils::multByConstNew(c1, cipher.c1, cnst, qi, params.n);

//	ZZ eBnd = cipher.eBnd * cnst;
//	ZZ mBnd = cipher.mBnd * cnst;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

Cipher Scheme::multByConstNew(Cipher& cipher, CZZ& cnst) {
	ZZ qi = getqi(cipher.level);
	CZZX c0, c1;
	Ring2Utils::multByConstNew(c0, cipher.c0, cnst, qi, params.n);
	Ring2Utils::multByConstNew(c1, cipher.c1, cnst, qi, params.n);
//	ZZ norm = cnst.norm();

//	ZZ eBnd = cipher.eBnd * norm;
//	ZZ mBnd = cipher.mBnd * norm;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

void Scheme::multByConstAndEqualNew(Cipher& cipher, ZZ& cnst) {
	ZZ qi = getqi(cipher.level);
	Ring2Utils::multByConstAndEqualNew(cipher.c0, cnst, qi, params.n);
	Ring2Utils::multByConstAndEqualNew(cipher.c1, cnst, qi, params.n);

//	cipher.eBnd *= cnst;
//	cipher.mBnd *= cnst;
}

void Scheme::multByConstAndEqualNew(Cipher& cipher, CZZ& cnst) {
	ZZ qi = getqi(cipher.level);
	Ring2Utils::multByConstAndEqualNew(cipher.c0, cnst, qi, params.n);
	Ring2Utils::multByConstAndEqualNew(cipher.c1, cnst, qi, params.n);

//	ZZ norm = cnst.norm();

//	cipher.eBnd *= norm;
//	cipher.mBnd *= norm;
}

Cipher Scheme::multByMonomial(Cipher& cipher, const long& degree) {
	CZZX c0 , c1;

	Ring2Utils::multByMonomial(c0, cipher.c0, degree, params.n);
	Ring2Utils::multByMonomial(c1, cipher.c1, degree, params.n);

//	Cipher res(c0, c1, cipher.level, cipher.eBnd, cipher.mBnd);
	Cipher res(c0, c1, cipher.level);
	return res;
}

void Scheme::multByMonomialAndEqual(Cipher& cipher, const long& degree) {
	Ring2Utils::multByMonomialAndEqual(cipher.c0, degree, params.n);
	Ring2Utils::multByMonomialAndEqual(cipher.c1, degree, params.n);
}

Cipher Scheme::multByMonomialNew(Cipher& cipher, const long& degree) {
	CZZX c0 , c1;

	Ring2Utils::multByMonomialNew(c0, cipher.c0, degree, params.n);
	Ring2Utils::multByMonomialNew(c1, cipher.c1, degree, params.n);

//	Cipher res(c0, c1, cipher.level, cipher.eBnd, cipher.mBnd);
	Cipher res(c0, c1, cipher.level);
	return res;
}

void Scheme::multByMonomialAndEqualNew(Cipher& cipher, const long& degree) {
	Ring2Utils::multByMonomialAndEqualNew(cipher.c0, degree, params.n);
	Ring2Utils::multByMonomialAndEqualNew(cipher.c1, degree, params.n);
}

Cipher Scheme::leftShift(Cipher& cipher, long& bits) {
	long logqi = getLogqi(cipher.level);
	CZZX c0, c1;

	Ring2Utils::leftShift(c0, cipher.c0, bits, logqi, params.n);
	Ring2Utils::leftShift(c1, cipher.c1, bits, logqi, params.n);

//	ZZ eBnd = cipher.eBnd << bits;
//	ZZ mBnd = cipher.mBnd << bits;

//	Cipher newCipher(c0, c1, cipher.level, eBnd, mBnd);
	Cipher newCipher(c0, c1, cipher.level);
	return newCipher;
}

void Scheme::leftShiftAndEqual(Cipher& cipher, long& bits) {
	long logqi = getLogqi(cipher.level);
	Ring2Utils::leftShiftAndEqual(cipher.c0, bits, logqi, params.n);
	Ring2Utils::leftShiftAndEqual(cipher.c1, bits, logqi, params.n);

//	cipher.eBnd <<= bits;
//	cipher.mBnd <<= bits;
}

Cipher Scheme::modSwitch(Cipher& cipher, long newLevel) {
	long logdf = params.logp * (newLevel-cipher.level);
	CZZX c0, c1;

	Ring2Utils::rightShift(c0, cipher.c0, logdf, params.n);
	Ring2Utils::rightShift(c1, cipher.c1, logdf, params.n);

//	ZZ eBnd = (cipher.eBnd >> params.logp) + params.Bscale;
//	ZZ mBnd = cipher.mBnd >> params.logp;

//	Cipher newCipher(c0, c1, newLevel, eBnd, mBnd);
	Cipher newCipher(c0, c1, newLevel);
	return newCipher;
}

Cipher Scheme::modSwitch(Cipher& cipher) {
	long newLevel = cipher.level + 1;
	return modSwitch(cipher, newLevel);
}

void Scheme::modSwitchAndEqual(Cipher& cipher, long newLevel) {
	long logdf = params.logp * (newLevel-cipher.level);
	Ring2Utils::rightShiftAndEqual(cipher.c0, logdf, params.n);
	Ring2Utils::rightShiftAndEqual(cipher.c1, logdf, params.n);
	cipher.level = newLevel;

//	cipher.eBnd >>= params.logp;
//	cipher.eBnd += params.Bscale;
//	cipher.mBnd >>= params.logp;
}

void Scheme::modSwitchAndEqual(Cipher& cipher) {
	long newLevel = cipher.level + 1;
	modSwitchAndEqual(cipher, newLevel);
}

Cipher Scheme::modEmbed(Cipher& cipher, long newLevel) {
	long newLogqi = getLogqi(newLevel);
	CZZX c0, c1;
	Ring2Utils::truncate(c0, cipher.c0, newLogqi, params.n);
	Ring2Utils::truncate(c1, cipher.c1, newLogqi, params.n);
//	Cipher newCipher(c0, c1, newLevel, cipher.eBnd, cipher.mBnd);
	Cipher newCipher(c0, c1, newLevel);
	return newCipher;
}

Cipher Scheme::modEmbed(Cipher& cipher) {
	long newLevel = cipher.level + 1;
	return modEmbed(cipher, newLevel);
}

void Scheme::modEmbedAndEqual(Cipher& cipher, long newLevel) {
	long newLogqi = getLogqi(newLevel);
	Ring2Utils::truncateAndEqual(cipher.c0, newLogqi, params.n);
	Ring2Utils::truncateAndEqual(cipher.c0, newLogqi, params.n);
	cipher.level = newLevel;
}

void Scheme::modEmbedAndEqual(Cipher& cipher) {
	long newLevel = cipher.level + 1;
	modEmbedAndEqual(cipher, newLevel);
}

Cipher Scheme::multAndModSwitch(Cipher& cipher1, Cipher& cipher2) {
	Cipher c = mult(cipher1, cipher2);
	modSwitchAndEqual(c);
	return c;
}

void Scheme::multModSwitchAndEqual(Cipher& cipher1, Cipher& cipher2) {
	multAndEqual(cipher1, cipher2);
	modSwitchAndEqual(cipher1);
}

Cipher Scheme::multAndModSwitchNew(Cipher& cipher1, Cipher& cipher2) {
	Cipher c = multNew(cipher1, cipher2);
	modSwitchAndEqual(c);
	return c;
}

void Scheme::multModSwitchAndEqualNew(Cipher& cipher1, Cipher& cipher2) {
	multAndEqualNew(cipher1, cipher2);
	modSwitchAndEqual(cipher1);
}

Cipher Scheme::squareAndModSwitch(Cipher& cipher) {
	Cipher c = square(cipher);
	modSwitchAndEqual(c);
	return c;
}

void Scheme::squareModSwitchAndEqual(Cipher& cipher) {
	squareAndEqual(cipher);
	modSwitchAndEqual(cipher);
}

//----------------------------------

ZZ Scheme::getqi(long& level) {
	return params.qi[params.L - level];
}

ZZ Scheme::getPqi(long& level) {
	return params.Pqi[params.L - level];
}

long Scheme::getLogqi(long& level) {
	return params.logq - params.logp * (level-1);
}
