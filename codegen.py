#!/usr/bin/env python
#-*- coding: utf-8 -*-
import logics

if __name__ == "__main__":
	logics.vistache.main(
		startDelimiter="/*(",
		endDelimiter=")*/",
		emptyValue="",
		startBlock="on",
		altBlock="or",
		endBlock="go"
	)

