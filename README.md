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

Status
======

Work still very much in progress. Watch this space!

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
    cmake ../posdtgresbson
    make
    make install # may require sudo
    make test

See also
========

*  PostgreSQL - http://www.postgresql.org/
*  PLV8 - http://code.google.com/p/plv8js/wiki/PLV8
*  Building a MongoDB Clone in Postgres - http://legitimatesounding.com/blog/building_a_mongodb_clone_in_postgres_part_1.html
*  BSON - http://bsonspec.org/
*  MongoDB - http://mongodb.org



