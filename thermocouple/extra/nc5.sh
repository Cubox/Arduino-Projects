#!/bin/sh

nc -l -k -w 5 4205 | tee -a log5
