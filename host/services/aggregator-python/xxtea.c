/*
	Python extension for XXTEA-based encryption

    Copyright (C) 2008 Istituto per l'Interscambio Scientifico I.S.I.
    You can contact us by email (isi@isi.it) or write to:
    ISI Foundation, Viale S. Severo 65, 10133 Torino, Italy. 

    This program was written by Ciro Cattuto <ciro.cattuto@gmail.com>

    Based on "Correction to xtea" by David J. Wheeler and Roger M. Needham.
    Computer Laboratory, Cambridge University (1998)
    http://www.movable-type.co.uk/scripts/xxtea.pdf

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <Python.h>
#include <netinet/in.h>

static u_int32_t tea_key[4] = {0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF};

static PyObject *
set_key(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "IIII", tea_key, tea_key+1, tea_key+2, tea_key+3))
        return NULL;

	Py_INCREF(Py_None);
    return Py_None;
}


#define MX ( (((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(tea_key[(p&3)^e]^z)) )
#define DELTA 0x9E3779B9L

static PyObject *
decode(PyObject *self, PyObject *args)
{
	PyObject *encoded_buf_obj, *decoded_buf_obj;
	u_int32_t *data, *v;
	int n;
    u_int32_t z, y, e, p, sum;

    if (!PyArg_ParseTuple(args, "O", &encoded_buf_obj))
        return NULL;

	if ( PyObject_AsReadBuffer(encoded_buf_obj, (const void **) &data, (Py_ssize_t *) &n) )
		return NULL;

	if (n % 4) {
		PyErr_SetString(PyExc_ValueError, "length must be a multiple of 4 bytes");
		return NULL;
	}

	if (n < 8) {
		PyErr_SetString(PyExc_ValueError, "length must be at least 8 bytes");
		return NULL;
	}

	decoded_buf_obj = PyBuffer_New((Py_ssize_t) n);
	if (decoded_buf_obj == NULL)
		return NULL;

    if ( PyObject_AsWriteBuffer(decoded_buf_obj, (void **) &v, (Py_ssize_t *) &n) )
		return NULL;

	n /= 4;

	for (p=0; p<n; p++)
		v[p] = ntohl(data[p]);

	y = v[0];
    sum = (6 + 52/n) * DELTA;
    while (sum != 0) {
		e = (sum>>2) & 3;
		for (p=n-1; p>0; p--) {
	    	z = v[p-1];
	    	y = v[p] -= MX;
		}
		z = v[n-1];
		y = v[0] -= MX;
		sum -= DELTA;
    }

	for (p=0; p<n; p++)
		v[p] = htonl(v[p]);

    return decoded_buf_obj;
}

static PyObject *
encode(PyObject *self, PyObject *args)
{
	PyObject *cleartext_buf_obj, *encoded_buf_obj;
	u_int32_t *data, *v;
	int n;
    u_int32_t z, y, e, p, q, sum;

    if (!PyArg_ParseTuple(args, "O", &cleartext_buf_obj))
        return NULL;

	if ( PyObject_AsReadBuffer(cleartext_buf_obj, (const void **) &data, (Py_ssize_t *) &n) )
		return NULL;

	if (n % 4) {
		PyErr_SetString(PyExc_ValueError, "length must be a multiple of 4 bytes");
		return NULL;
	}

	if (n < 8) {
		PyErr_SetString(PyExc_ValueError, "length must be at least 8 bytes");
		return NULL;
	}

	encoded_buf_obj = PyBuffer_New((Py_ssize_t) n);
	if (encoded_buf_obj == NULL)
		return NULL;

    if ( PyObject_AsWriteBuffer(encoded_buf_obj, (void **) &v, (Py_ssize_t *) &n) )
		return NULL;

	n /= 4;

	for (p=0; p<n; p++)
		v[p] = ntohl(data[p]);

	y = v[0];
	z = v[n-1];
	q = 6 + 52/n;
	sum = 0;
	while (q-- > 0) {
		sum += DELTA;
		e = (sum >> 2) & 3;
		for (p=0; p<n-1; p++) {
			y = v[p+1];
			z = v[p] += MX;
		}
		y = v[0];
		z = v[n-1] += MX;
	}

	for (p=0; p<n; p++)
		v[p] = htonl(v[p]);

    return encoded_buf_obj;
}

static PyObject *
crc16(PyObject *self, PyObject *args)
{
	u_int16_t crc = 0xFFFF;
	unsigned char *buffer;
	unsigned long n;

    if (!PyArg_ParseTuple(args, "s#", &buffer, &n))
        return NULL;

    if (n == 0) {
		Py_INCREF(Py_None);
    	return Py_None;
	}

	while (n--) {
  		crc = (crc >> 8) | (crc << 8);
      	crc ^= *buffer++;
      	crc ^= (crc & 0xFF) >> 4;
      	crc ^= crc << 12;
      	crc ^= (crc & 0xFF) << 5;
	}

	return Py_BuildValue("i", crc);
}


static PyMethodDef XXTEAMethods[] = {
    {"set_key", set_key, METH_VARARGS, "set_key"},
    {"decode", decode, METH_VARARGS, "decode"},
    {"encode", encode, METH_VARARGS, "encode"},
    {"crc16", crc16, METH_VARARGS, "crc16"},
    {NULL, NULL, 0, NULL} 
};

PyMODINIT_FUNC
initxxtea(void)
{
    (void) Py_InitModule("xxtea", XXTEAMethods);
}

