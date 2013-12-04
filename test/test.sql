\qecho * loading extension
CREATE EXTENSION pgbson;

\qecho * creating tables
CREATE TEMPORARY TABLE data_table (
    id BIGSERIAL,
    data BSON,

    PRIMARY KEY(id)
);

CREATE TEMPORARY TABLE results_table (
    name TEXT NOT NULL,
    expected TEXT,
    got TEXT
);

\qecho * testing extension version
INSERT INTO results_table(name, got, expected)
VALUES ('pgbson version', pgbson_version(), '1.0');


\qecho * json format input
INSERT INTO data_table(id, data)
VALUES
(1, '{"string_field":"from json", "integer_field":42, "int64_filed" : 1099511627776, "float": 3.14, "nested":{"ns":"boo"}}');

INSERT INTO data_table(id, data)
VALUES
(3, '{"array1": [ 1, 2, 3, 4, 5 ], "array2" : ["a", "b", "c"], "array3" : [{"a": 1 }, { "b" : 2.3 } ], "scalar_int" : 42 }');

\qecho * bson from row

CREATE TYPE nested_obj_type AS (ns TEXT);
CREATE TYPE obj_type AS (string_field TEXT, integer_field INTEGER, nested BSON);


INSERT INTO data_table(id, data)
VALUES
(2, row_to_bson(
    row(
        'from row',
        42,
        row_to_bson(row('boo')::nested_obj_type)
        )::obj_type
    )
);

\qecho * Object inspection


INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_string on bson from json',
    'from json', bson_get_text(data, 'string_field') FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_string on bson from row',
    'from row', bson_get_text(data, 'string_field')  FROM data_table WHERE id = 2;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_bson on string',
    '{"":"from json"}'::bson::text, bson_get_bson(data, 'string_field')::text  FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_bson on nested object',
    '{"ns":"boo"}'::bson::text, bson_get_bson(data, 'nested')::text  FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_int on bson from json',
    42::text, bson_get_int(data, 'integer_field')::text  FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_double on bson from json',
    3.14::text, bson_get_double(data, 'float')::text  FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_double on bson from json, integer field',
    42::text, bson_get_double(data, 'integer_field')::text  FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_bigint on bson from json',
    1099511627776::text, bson_get_bigint(data, 'int64_filed')::text  FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_bigint on bson from json, integer field',
    42::text, bson_get_bigint(data, 'integer_field')::text  FROM data_table WHERE id = 1;

\qecho * Array tools

INSERT INTO results_table(name, expected, got)
SELECT 'bson_array_size on array with 5 integers',
    5::text, bson_array_size(data, 'array1')::text  FROM data_table WHERE id = 3;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_array_size on array with 3 strings',
    3::text, bson_array_size(data, 'array2')::text  FROM data_table WHERE id = 3;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_array_size on array with 2 objects',
    2::text, bson_array_size(data, 'array3')::text  FROM data_table WHERE id = 3;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_array_size on array scalar',
    1::text, bson_array_size(data, 'scalar_int')::text  FROM data_table WHERE id = 3;

\qecho * Unwind array

CREATE TEMPORARY TABLE unwound_arrays
(
    name TEXT,
    item BSON
);

INSERT INTO unwound_arrays(name, item)
SELECT 'array of 5 integers', bson_unwind_array(data, 'array1') FROM data_table WHERE id = 3;

INSERT INTO unwound_arrays(name, item)
SELECT 'array of 3 strings', bson_unwind_array(data, 'array2') FROM data_table WHERE id = 3;

INSERT INTO unwound_arrays(name, item)
SELECT 'array of 2 objects', bson_unwind_array(data, 'array3') FROM data_table WHERE id = 3;

INSERT INTO unwound_arrays(name, item)
SELECT 'scalar', bson_unwind_array(data, 'scalar_int') FROM data_table WHERE id = 3;

INSERT INTO unwound_arrays(name, item)
SELECT 'not existing', bson_unwind_array(data, 'nope') FROM data_table WHERE id = 3;

SELECT * FROM unwound_arrays;

\qecho * Operators

INSERT INTO results_table(name, expected, got)
SELECT 'equality on identical objects', true, '{"a":3, "b":"boo"}'::bson = '{ "a" : 3, "b" : "boo" }'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'equality on different objects', false, '{"a":3, "b":"boo"}'::bson = '{ "a" : 7, "X":{"f":"ghj"}}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'equality on reordered fields', false, '{"a":3, "b":"boo"}'::bson = '{"b":"boo", "a":3}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'inequality on identical objects', false, '{"a":3, "b":"boo"}'::bson <> '{ "a" : 3, "b" : "boo" }'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'inequality on different objects', true, '{"a":3, "b":"boo"}'::bson <> '{ "a" : 7, "X":{"f":"ghj"}}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT '==', false, row_to_bson(row(3::float)) == row_to_bson(row(3::integer));

INSERT INTO results_table(name, expected, got)
SELECT '<<>>', true, row_to_bson(row(3::float)) <<>> row_to_bson(row(3::integer));



INSERT INTO results_table(name, expected, got)
SELECT 'lt-lt', true, '{"a":3}'::bson < '{"a":4}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'lt-eq', false, '{"a":3}'::bson < '{"a":3}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'lt-gt', false, '{"a":3}'::bson < '{"a":2}'::bson;



INSERT INTO results_table(name, expected, got)
SELECT 'lte-lt', true, '{"a":3}'::bson <= '{"a":4}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'lte-eq', true, '{"a":3}'::bson <= '{"a":3}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'lte-gt', false, '{"a":3}'::bson <= '{"a":2}'::bson;



INSERT INTO results_table(name, expected, got)
SELECT 'gt-lt', false, '{"a":3}'::bson > '{"a":4}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'gt-eq', false, '{"a":3}'::bson > '{"a":3}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'gt-gt', true, '{"a":3}'::bson > '{"a":2}'::bson;



INSERT INTO results_table(name, expected, got)
SELECT 'gte-lt', false, '{"a":3}'::bson >= '{"a":4}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'gte-eq', true, '{"a":3}'::bson >= '{"a":3}'::bson;

INSERT INTO results_table(name, expected, got)
SELECT 'gte-gt', true, '{"a":3}'::bson >= '{"a":2}'::bson;

\qecho * hash index creation
CREATE INDEX test_hash_idx ON data_table USING hash (bson_get_bson(data, '_id'));

\qecho * btree index creation
CREATE INDEX test_btree_idx ON data_table USING btree (bson_get_bson(data, '_id'));

-- this must be at the end
\qecho * test results, check for failures!
SELECT *, expected = got AS passed FROM results_table;
