pypoly: Python Polynomials 
==========================

A Python extension written in C, implementing the `Polynomial` type.

Installation
------------

*Requires Python 3*

    python setup.py install

Or:

    pip install PyPolynomial

Usage
-----

**Instanciating a polynomial:**

    >>> from pypoly import Polynomial
    >>> P = Polynomial(-1, 0, complex(1, 3))
    >>> P
    -1 + (1+3j) * X**2
    >>> P.degree
    2

Or:

    >>> from pypoly import X
    >>> P = (X - 1) * (X**2 + 2)
    >>> P
    -2 + 2 * X - X**2 + X**3

**Operations on polynomials:**

    >>> Polynomial(-1, -1) * Polynomial(2, -3)
    -2 + X + 3 * X**2
    >>> P = Polynomial(1, 2, 3)
    >>> P(13)
    534.0
    >>> (1 + X + X**2) // (1 + X)
    X
    >>> (2 * X + 3 * X**2 + X**5 + X**7) % (X**2 + 1)
    -3 + 2 * X
    >>> (1 + X**3 - 2 * X**5) >> 2      # Second derivative
    6 * X - 40 * X**3

Links
-----

Found a bug ? Have a feature request ? Need help ? [Ask here](https://github.com/tchaumeny/PyPoly/issues/new "New issue")

Project page on PyPI: [https://pypi.python.org/pypi/PyPolynomial](https://pypi.python.org/pypi/PyPolynomial)

[![Build Status](https://travis-ci.org/tchaumeny/PyPoly.png)](https://travis-ci.org/tchaumeny/PyPoly)
