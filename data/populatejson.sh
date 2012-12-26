#!/bin/bash
# Populates PostgreSQL table with data from pped JSON file (.json.gz, one record per line).
# Target field can be text, json (Postgres>9.2) or bson (with pgbson).

PSQL=psql
JSONFILE=$1
DBNAME=$2
TABLENAME=$3
FIELDNAME=$4

if [ x$FIELDNAME = x ]
then
    echo 'USAGE: populatejson.sh INFILE DBNAME TABLENAME FIELDNAME'
    exit 1
fi

(echo "COPY $TABLENAME ($FIELDNAME) FROM STDIN WITH (FORMAT TEXT);" ; zcat $JSONFILE | sed 's/\\/\\\\/g') | $PSQL $DBNAME
