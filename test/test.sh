#!/bin/bash

PSQL=psql
CREATEDB=createdb
DROPDB=dropdb
TESTDB=pgbson_test

$CREATEDB $TESTDB
$PSQL $TESTDB < test.sql
$DROPDB $TESTDB
