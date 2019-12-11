#!/bin/sh

nc -l -k -w 5 4203 | tee -a log3
