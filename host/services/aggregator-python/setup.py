# Python extension for XXTEA-based encryption

from distutils.core import setup, Extension

xxtea_module = Extension('xxtea', sources = ['xxtea.c'])
setup (name = 'xxtea', version = '0.1', description = 'XXTEA encryption and utils', ext_modules=[xxtea_module])

