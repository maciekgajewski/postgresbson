postgresbson
============

BSON support for PostgreSQL

Introduction
============

This PostgreSQL extension brings BSON data type, together with functions to create, inspect and manipulate BSON objects.

BSON (http://bsonspec.org/) is a document serialization format used by MongoDB.
This extension allows for smoother migration path from MongoDB to PostgreSQL.

Additionaly, parsing BSON is much faster than JSON. Preliminary bechnmarks show that accessing BSON
field with this extension is 25x faster than accessing JSON field with PLv8.

Still, the PLv8 and JSON functions and operators may be used to build and manipulate BSON data.

Status
======

Stable, used in production.


Example
=======

    CREATE EXTENSION pgbson;
    CREATE TABLE data_collection ( data BSON );
    CREATE INDEX ON data_collection( bson_get_text(data, 'name'));
    
    INSERT INTO data_collection VALUES ('{"name":"John", "age":32}'), ('{"name":"Spike", "spieces":"Dog"}');
    
    select * from data_collection where bson_get_text(data, 'name') = 'Spike';
                      data                   
    -----------------------------------------                                                                                                                                                            
    { "name" : "Spike", "spieces" : "Dog" }                                                                                                                                                             
    (1 row)      
    
    

Building
========

Tested on on Linux with Postgres 9.2. Should work with any Postgres 9.x.
Requires: CMake, Boost, pg_config, C++ compiler

    git clone https://github.com/maciekgajewski/postgresbson.git
    mkdir postgresbson-build
    cd postgresbson-build
    cmake ../postgresbson
    make
    make install # may require sudo
    make test


Quick reference
===============

The module defines BSON data type with operator families defined for B-TREE and HASH indexes.

Operators and comparison:

*  Operators: =, <>, <=, <, >=, >, == (binary equality), <<>> (binary inequality)
*  bson_hash(bson) RETURNS INT4

Field access (supports dot notation):

*  bson_get_text(bson, text) RETURNS text
*  bson_get_int(bson, text) RETURNS int4
*  bson_get_double(bson, text) RETURNS float8
*  bson_get_bigint(bson, text) RETURNS int8
*  bson_get_bson(bson, text) RETURNS bson

Array field support:

*  bson_array_size(bson, text) RETURNS int8
*  bson_unwind_array(bson, text) RETURNS SETOF bson

Object construction:

*  row_to_bson(record) RETURNS bson

See also
========

*  PostgreSQL - http://www.postgresql.org/
*  PLV8 - http://code.google.com/p/plv8js/wiki/PLV8
*  Building a MongoDB Clone in Postgres - http://legitimatesounding.com/blog/building_a_mongodb_clone_in_postgres_part_1.html
*  BSON - http://bsonspec.org/
*  MongoDB - http://mongodb.org



