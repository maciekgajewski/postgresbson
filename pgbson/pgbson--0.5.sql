------------------------------------
-- type definition and i/o functions
------------------------------------

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

------------
-- operators
------------

-- logical comparison
CREATE FUNCTION bson_compare(bson, bson) RETURNS INT4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION bson_equal(bson, bson) RETURNS BOOL AS $$
    SELECT bson_compare($1, $2) = 0;
$$ LANGUAGE SQL;

CREATE FUNCTION bson_not_equal(bson, bson) RETURNS BOOL AS $$
    SELECT bson_compare($1, $2) <> 0;
$$ LANGUAGE SQL;

CREATE FUNCTION bson_lt(bson, bson) RETURNS BOOL AS $$
    SELECT bson_compare($1, $2) < 0;
$$ LANGUAGE SQL;

CREATE FUNCTION bson_lte(bson, bson) RETURNS BOOL AS $$
    SELECT bson_compare($1, $2) <= 0;
$$ LANGUAGE SQL;

CREATE FUNCTION bson_gt(bson, bson) RETURNS BOOL AS $$
    SELECT bson_compare($1, $2) > 0;
$$ LANGUAGE SQL;

CREATE FUNCTION bson_gte(bson, bson) RETURNS BOOL AS $$
    SELECT bson_compare($1, $2) >= 0;
$$ LANGUAGE SQL;

CREATE OPERATOR = (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_equal,
    NEGATOR = <>
);

CREATE OPERATOR <> (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_not_equal,
    NEGATOR = =
);

CREATE OPERATOR < (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_lt,
    NEGATOR = >=
);

CREATE OPERATOR <= (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_lte,
    NEGATOR = >
);

CREATE OPERATOR > (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_gt,
    NEGATOR = <=
);

CREATE OPERATOR >= (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_gte,
    NEGATOR = <
);

-- binary equality
CREATE FUNCTION bson_binary_equal(bson, bson) RETURNS BOOL
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION bson_binary_not_equal(bson, bson) RETURNS BOOL AS $$
    SELECT NOT(bson_binary_equal($1, $2));
$$ LANGUAGE SQL;


CREATE OPERATOR == (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_binary_equal,
    NEGATOR = <<>>
);

CREATE OPERATOR <<>> (
    LEFTARG = bson,
    RIGHTARG = bson,
    PROCEDURE = bson_binary_not_equal,
    NEGATOR = ==
);

---------------------
-- hash index support
---------------------

CREATE FUNCTION bson_hash(bson) RETURNS INT4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR CLASS bson_hash_ops
    DEFAULT FOR TYPE bson USING hash AS
        OPERATOR 1 == (bson, bson) ,
        FUNCTION 1 bson_hash(bson);

-----------------------
-- b-tree index support
-----------------------

CREATE OPERATOR CLASS bson_btree_ops
    DEFAULT FOR TYPE bson USING btree AS
        OPERATOR 1 < (bson, bson),
        OPERATOR 2 <= (bson, bson),
        OPERATOR 3 = (bson, bson),
        OPERATOR 4 >= (bson, bson),
        OPERATOR 5 > (bson, bson),
        FUNCTION 1 bson_compare(bson, bson);

------------------
-- other functions
------------------

CREATE FUNCTION pgbson_version() RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

------------------------------
-- deep object inspection functions
------------------------------

-- returns (dotted) field value converted to text. Works only on scalar types: string, numbers, Oid, date
-- returns null if no such field
-- fails if conversion is impossible
CREATE FUNCTION bson_get_text(bson, text) RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

-- returns (dotted) field as bson object.
-- scalars are returned as bson objects with single, anonymous field.
-- returns null if no such field
CREATE FUNCTION bson_get_as_bson(bson, text) RETURNS bson
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;


--------------------------
-- conversion to/from bson
--------------------------

-- converts row to bson, very much like row_to_json
CREATE FUNCTION row_to_bson(record) RETURNS bson
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;
