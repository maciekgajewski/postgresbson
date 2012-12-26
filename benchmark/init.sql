-- V8
CREATE EXTENSION IF NOT EXISTS plv8;
CREATE OR REPLACE FUNCTION json_get_string(data json, key varchar) RETURNS
VARCHAR AS $$
  var obj = JSON.parse(data);
  var parts = key.split('.');

  var part = parts.shift();
  while (part && (obj = obj[part]) !== undefined) {
    part = parts.shift();
  }

  // this will either be the value, or undefined
  return obj;
$$ LANGUAGE plv8 STRICT IMMUTABLE;



DROP TABLE IF EXISTS jsontest;
CREATE TABLE jsontest (id bigserial, d json);

-- pgbson
CREATE EXTENSION IF NOT EXISTS pgbson;

DROP TABLE IF EXISTS bsontest;
CREATE TABLE bsontest (id bigserial, d bson);
