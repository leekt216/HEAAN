#include "EvaluatorUtils.h"

#include <cmath>
#include <cstdlib>

#include "NumUtils.h"

CZZ EvaluatorUtils::evaluateVal(const double& xr, const double& xi, const long& logp) {
	if(logp < 31) {
		long p = 1 << logp;
		ZZ pxexpr = to_ZZ(xr * p);
		ZZ pxexpi = to_ZZ(xi * p);
		return CZZ(pxexpr, pxexpi);
	} else {
		long tmp = (1 << 30);
		ZZ pxexpr = to_ZZ(xr * tmp) << (logp - 30);
		ZZ pxexpi = to_ZZ(xi * tmp) << (logp - 30);
		return CZZ(pxexpr, pxexpi);
	}
}

CZZ EvaluatorUtils::evaluateRandomVal(const long& logp) {
	ZZ tmpr, tmpi;
	RandomBits(tmpr, logp);
	RandomBits(tmpi, logp);
	return CZZ(tmpr, tmpi);
}

CZZ EvaluatorUtils::evaluateRandomCircleVal(const long& logp) {
	double angle = (double)arc4random() / RAND_MAX;
	double mr = cos(angle * 2 * Pi);
	double mi = sin(angle * 2 * Pi);
	return evaluateVal(mr, mi, logp);
}

CZZ* EvaluatorUtils::evaluateRandomVals(const long& size, const long& logp) {
	return NumUtils::sampleUniform2(size, logp);
}

CZZ EvaluatorUtils::evaluatePow(const double& xr, const double& xi, const long& degree, const long& logp) {
	long logDegree = log2(degree);
	long po2Degree = 1 << logDegree;
	CZZ res = evaluatePow2(xr, xi, logDegree, logp);
	long remDegree = degree - po2Degree;
	if(remDegree > 0) {
		CZZ tmp = evaluatePow(xr, xi, remDegree, logp);
		res *= tmp;
		res >>= logp;
	}
	return res;
}

CZZ EvaluatorUtils::evaluatePow2(const double& xr, const double& xi, const long& logDegree, const long& logp) {
	CZZ res = evaluateVal(xr, xi, logp);
	for (int i = 0; i < logDegree; ++i) {
		res = (res * res) >> logp;
	}
	return res;
}

CZZ* EvaluatorUtils::evaluatePowvec(const double& xr, const double& xi, const long& degree, const long& logp) {
	CZZ* res = new CZZ[degree];
	CZZ m = evaluateVal(xr, xi, logp);
	res[0] = m;
	for (long i = 0; i < degree - 1; ++i) {
		res[i + 1] = (res[i] * m) >> logp;
	}
	return res;
}

CZZ* EvaluatorUtils::evaluatePow2vec(const double& xr, const double& xi, const long& logDegree, const long& logp) {
	CZZ* res = new CZZ[logDegree + 1];
	CZZ m = evaluateVal(xr, xi, logp);
	res[0] = m;
	for (long i = 0; i < logDegree; ++i) {
		res[i + 1] = (res[i] * res[i]) >> logp;
	}
	return res;
}

CZZ EvaluatorUtils::evaluateInverse(const double& xr, const double& xi, const long& logp) {
	double xinvr = xr / (xr * xr + xi * xi);
	double xinvi = -xi / (xr * xr + xi * xi);

	return evaluateVal(xinvr, xinvi, logp);
}

CZZ EvaluatorUtils::evaluateExponent(const double& xr, const double& xi, const long& logp) {
	double xrexp = exp(xr);
	double xexpr = xrexp * cos(xi);
	double xexpi = xrexp * sin(xi);

	return evaluateVal(xexpr, xexpi, logp);
}

CZZ EvaluatorUtils::evaluateSigmoid(const double& xr, const double& xi, const long& logp) {
	double xrexp = exp(xr);
	double xexpr = xrexp * cos(xi);
	double xexpi = xrexp * sin(xi);

	double xsigmoidr = (xexpr * (xexpr + 1) + (xexpi * xexpi)) / ((xexpr + 1) * (xexpr + 1) + (xexpi * xexpi));
	double xsigmoidi = xexpi / ((xexpr + 1) * (xexpr + 1) + (xexpi * xexpi));

	return evaluateVal(xsigmoidr, xsigmoidi, logp);
}

void EvaluatorUtils::leftShift(CZZ*& vals, const long& size, const long& logp) {
	for (long i = 0; i < size; ++i) {
		vals[i] <<= logp;
	}
}

void EvaluatorUtils::idxShift(CZZ*& vals, const long& size, const long& shift) {
	CZZ* tmp = new CZZ[size];
	for (long i = 0; i < size - shift; ++i) {
		tmp[i] = vals[i + shift];
	}
	for (long i = size - shift; i < size; ++i) {
		tmp[i] = vals[i + shift - size];
	}
	vals = tmp;
}
