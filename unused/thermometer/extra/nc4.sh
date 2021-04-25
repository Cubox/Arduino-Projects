#!/bin/sh

nc -l -k -w 5 4204 | tee -a log4
