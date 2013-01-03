#!/bin/bash

PSQL=psql
CREATEDB=createdb
DROPDB=dropdb
TESTDB=pgbson_test

# clean-up after previous, possibly db-crashing test
$DROPDB --if-exists $TESTDB

# create -> run -> drop
$CREATEDB $TESTDB
$PSQL $TESTDB < test.sql
$DROPDB $TESTDB
