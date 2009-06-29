/*
 *  ADataVector.h
 *  Acinonyx
 *
 *  Created by Simon Urbanek on 5/4/09.
 *  Copyright 2009 Simon Urbanek. All rights reserved.
 *
 */

/******** THIS IS CURRENTLY UNUSED - ADataVector is defined in AVector.h *********/

#ifndef A_DATAVECTOR_H
#define A_DATAVECTOR_H

#include "AVector.h"
#include "ANotfier.h"
#include "AMarker.h"

/*
class AObjectWithMarker {
protected:
	AMarker *_marker;
 
public:
	AObjectWithMarker(AMarker *m) : _marker(NULL) { if (m) { _marker = m; ((AObject*)_marker)->retain(); } }
	virtual ~AObjectWithMarker() { if (_marker) ((AObject*)_marker)->release(); }
 
	AMarker *marker() { return _marker; }
};

class ADataVector : public AVector, public AObjectWithMarker {
 */

class ADataVector : public AVector, public ANotifierInterface {
protected:
	AMarker *_marker;
	char *_name;
public:
	ADataVector(AMarker *mark, vsize_t len, const char *name = NULL) : AVector(len), ANotifierInterface(false), _marker(mark) {
		_name = name ? strdup(name) : NULL;
		// FIXME: we have a problem here with retaining the marker - for now we require that markers must be retained separately from their users ..
		//if (_marker) AObject_retain(_marker);
		if (_marker) _marker->retain();
		OCLASS(ADataVector);
	};
	virtual ~ADataVector() {
		if (_name) free(_name);
		//if (_marker) AObject_release(_marker);
		if (_marker) _marker->release();
		DCLASS(ADataVector);
	}
	
	AMarker *marker() { return _marker; }
	const char *name() { return _name; }
	void setName(const char *newName) { if (_name) free(_name); _name = strdup(newName); }
};

class AFloatVector : public ADataVector {
protected:
	float *_data;
	double *d_data;
	int *i_data;
public:
	AFloatVector(AMarker *m, float *data, vsize_t len, bool copy) : ADataVector(m, len), d_data(0), i_data(0) {
		_data = (float*) (copy?memdup(data, len * sizeof(float)):data); OCLASS(AFloatVector)
	}
	AFloatVector(float *data, vsize_t len, bool copy) : ADataVector(0, len), d_data(0), i_data(0) {
		_data = (float*) (copy?memdup(data, len * sizeof(float)):data); OCLASS(AFloatVector)
	}
	AFloatVector(float *data, vsize_t len) : ADataVector(0, len), d_data(0), i_data(0) {
		_data = (float*) memdup(data, len * sizeof(float)); OCLASS(AFloatVector)
	}
	
	virtual ~AFloatVector() {
		if (owned) free(_data);
		if (d_data) free(d_data);
		if (i_data) free(i_data);
		DCLASS(AFloatVector)
	}
	
	virtual ADataRange range() {
		ADataRange r = AUndefDataRange;
		if (length()) {
			double e = r.begin = _data[0];
			for (int i = 0; i < length(); i++)
				if (_data[i] < r.begin) r.begin = _data[i]; else if (_data[i] > e) e = _data[i];
			r.length = e - r.begin;
		}
		return r;
	}
	
	virtual const float *asFloats() { return _data; }
	virtual const double *asDoubles() {
		if (!d_data) {
			d_data = (double*) malloc(_len * sizeof(double));
			for (int i=0; i<_len; i++) d_data[i] = (double)_data[i];
		}
		return d_data;
	}
	virtual const int *asInts() {
		if (!i_data) {
			i_data = (int*) malloc(_len * sizeof(int));
			for (int i=0; i<_len; i++) i_data[i] = (int)_data[i];
		}
		return i_data;
	}
	
	virtual void transformToFloats(AFloat *f, float a, float b) { // a * data + b
		for (int i = 0; i < length(); i++)
			f[i] = _data[i] * a + b;
	}
	
	virtual void transformToDoubles(double *f, double a, double b) { // a * data + b
		for (int i = 0; i < length(); i++)
			f[i] = _data[i] * a + b;
	}
};

class ADoubleVector : public ADataVector {
protected:
	double *_data;
	float *f_data;
	int *i_data;
public:
	ADoubleVector(AMarker *m, double *data, vsize_t len, bool copy) : ADataVector(m, len), f_data(0), i_data(0) {
		_data = copy?(double*)memdup(data, len * sizeof(double)):data; OCLASS(ADoubleVector)
	}
	ADoubleVector(double *data, vsize_t len, bool copy) : ADataVector(0, len), f_data(0), i_data(0) {
		_data = copy?(double*)memdup(data, len * sizeof(double)):data; OCLASS(ADoubleVector)
	}
	ADoubleVector(const double *data, vsize_t len) : ADataVector(0, len), f_data(0), i_data(0) {
		_data = (double*)memdup(data, len * sizeof(double)); OCLASS(ADoubleVector)
	}	
	virtual ~ADoubleVector() {
		if (owned) free(_data);
		if (f_data) free(f_data);
		if (i_data) free(i_data);
		DCLASS(ADoubleVector)
	}
	
	virtual const double *asDoubles() { return _data; }		
	
	virtual ADataRange range() {
		ADataRange r = AUndefDataRange;
		if (length()) {
			double e = r.begin = _data[0];
			for (int i = 0; i < length(); i++)
				if (_data[i] < r.begin) r.begin = _data[i]; else if (_data[i] > e) e = _data[i];
			r.length = e - r.begin;
		}
		return r;
	}
	
	virtual const float *asFloats() {
		if (!f_data) {
			f_data = (float*) malloc(_len * sizeof(float));
			AMEM(f_data);
			for (vsize_t i = 0; i < _len; i++)
				f_data[i] = (float) _data[i];
		}
		return f_data;
	}
	
	virtual const int *asInts() {
		if (!i_data) {
			i_data = (int*) malloc(_len * sizeof(int));
			AMEM(i_data);
			for (vsize_t i = 0; i < _len; i++)
				i_data[i] = (int)_data[i];
		}
		return i_data;
	}
	
	virtual void transformToFloats(AFloat *f, float a, float b) { // a * data + b
		for (int i = 0; i < length(); i++)
			f[i] = _data[i] * a + b;
	}
	
	virtual void transformToDoubles(double *f, double a, double b) { // a * data + b
		for (int i = 0; i < length(); i++)
			f[i] = _data[i] * a + b;
	}
};

class AIntVector : public ADataVector {
protected:
	int *_data;
	double *d_data;
	float *f_data;
public:
	AIntVector(AMarker *m, const int *data, vsize_t len, bool copy) : ADataVector(m, len), f_data(0), d_data(0) {
		_data = (int*)(copy?memdup(data, len * sizeof(int)):data); OCLASS(AIntVector)
	}
	AIntVector(const int *data, vsize_t len, bool copy) : ADataVector(0, len), f_data(0), d_data(0) {
		_data = (int*)(copy?memdup(data, len * sizeof(int)):data); OCLASS(AIntVector)
	}
	AIntVector(const int *data, vsize_t len) : ADataVector(0, len), f_data(0), d_data(0) {
		_data = (int*)memdup(data, len * sizeof(int)); OCLASS(AIntVector)
	}
	
	virtual ~AIntVector() {
		if (owned) free(_data);
		if (d_data) free(d_data);
		if (f_data) free(f_data);
		DCLASS(AIntVector)
	}
	
	virtual const int *asInts() { return _data; }
	virtual const double *asDoubles() {
		if (!d_data) {
			d_data = (double*) malloc(_len * sizeof(double));
			for (int i=0; i<_len; i++) d_data[i] = (double)_data[i];
		}
		return d_data;
	}
	virtual const float *asFloats() {
		if (!f_data) {
			f_data = (float*) malloc(_len * sizeof(float));
			for (int i=0; i<_len; i++) f_data[i] = (float)_data[i];
		}
		return f_data;
	}
	
	virtual ADataRange range() {
		ADataRange r = AUndefDataRange;
		if (length()) {
			double e = r.begin = _data[0];
			for (int i = 0; i < length(); i++)
				if (_data[i] < r.begin) r.begin = _data[i]; else if (_data[i] > e) e = _data[i];
			r.length = e - r.begin;
		}
		return r;
	}
	
	virtual void transformToFloats(AFloat *f, float a, float b) { // a * data + b
		for (int i = 0; i < length(); i++)
			f[i] = ((float)_data[i]) * a + b;
	}
	
	virtual void transformToDoubles(double *f, double a, double b) { // a * data + b
		for (int i = 0; i < length(); i++)
			f[i] = ((double)_data[i]) * a + b;
	}	
};

class AUnivarTable : public AObject {
protected:
	vsize_t *_counts;
	vsize_t _size;
	vsize_t _other;
	vsize_t _max;
	char **_names;
public:
	AUnivarTable(vsize_t size, bool named=true) : _size(size), _names(NULL), _other(0), _max(0) {
		_counts = (vsize_t*) calloc(_size, sizeof(vsize_t));
		OCLASS(AUnivarTable)
	}
	
	virtual ~AUnivarTable() {
		free(_counts);
		DCLASS(AUnivarTable);
	}
	
	vsize_t size() { return _size; }
	
	vsize_t *counts() { return _counts; }
	
	vsize_t other() { return _other; }
	
	vsize_t maxCount() { return _max; }
	
	vsize_t count(vsize_t index) { return (index < _size) ? _counts[index] : 0; }
	
	char **names() { return _names; }
	
	char *name(vsize_t index) { return (_names && index < _size) ? _names[index] : 0; }
	
	void setName(vsize_t index, const char *name) {
		if (!_names) _names = (char**) calloc(_size, sizeof(char*));
		if (index < _size) {
			if (_names[index] && !strcmp(name, _names[index])) return;
			if (_names[index]) free(_names[index]);
			_names[index] = strdup(name);
		}
	}
	
	void reset() { memset(_counts, 0, sizeof(vsize_t) * _size); _other = 0; _max = 0; }
	
	void add(vsize_t entry) {
		if (entry < _size) {
			vsize_t c = ++_counts[entry];
			if (c > _max) _max = c;
		} else _other++;
	}
};

class AFactorVector : public AIntVector {
protected:
	char **_names;
	int _levels;
	char **s_data;
	AUnivarTable *_tab;
	APermutation *perm;
public:
	AFactorVector(AMarker *mark, const int *data, int len, const char **names, int n_len, bool copy=true) : AIntVector(mark, data, len, copy), _levels(n_len), _tab(0), perm(NULL) {
		_names = (char**) (copy ? memdup(names, n_len * sizeof(char*)) : names); OCLASS(AFactorVector)
	}
	virtual ~AFactorVector() {
		if (owned) {
			for (int i = 0; i < _levels; i++) if (_names[i]) free(_names[i]);
			free(_names);
		}
		if (perm) perm->release();
		if (_tab) _tab->release();
		DCLASS(AFactorVector)
	}
	virtual const char **asStrings() {
		if (!s_data) {
			s_data = (char**) malloc(_len * sizeof(char*));
			for (vsize_t i = 0; i < _len; i++)
				s_data[i] = (_data[i] < 0 || _data[i] >= _levels)?NULL:_names[_data[i]];
		}
		return (const char**) s_data;
	}
	
	virtual bool isFactor() { return true; }
	
	virtual APermutation *permutation() { return perm ? perm : (perm = new APermutation(_levels)); }
	
	// FIXME: we'll need to make it virtual in the super class ..
	virtual const char *stringAt(vsize_t i) {
		if (i >= _len) return NULL;
		int l = _data[i];
		return (l >= 0 && l < _levels) ?  _names[l] : NULL;
	}
	
	AUnivarTable *table() {
		if (!_tab) {
			_prof(profReport("^AFactorVector.table"))
			_tab = new AUnivarTable(_levels);
			for (vsize_t i = 0; i < _len; i++)
				_tab->add((vsize_t) _data[i]);
			if (_names) for (vsize_t i = 0; i < _levels; i++)
				_tab->setName(i, _names[i]);
			_prof(profReport("$AFactorVector.table"))
		}
		return _tab;
	}
	
	int levels() { return _levels; }
	char **levelStrings() { return _names; }
};

// TODO: mutable vectors ( + notification?)

#endif
