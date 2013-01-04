-- type definition
CREATE TYPE bson;

CREATE FUNCTION bson_in(cstring) RETURNS bson
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION bson_out(bson) RETURNS cstring
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE TYPE bson (
    input = bson_in,
    output = bson_out,
    alignment = int4,
    storage = main
);

-- object inspection

CREATE FUNCTION bson_get_text(bson, text) RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION row_to_bson(record) RETURNS bson
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;
