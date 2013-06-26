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
VALUES ('pgbson version', pgbson_version(), '0.5');


\qecho * json format input
INSERT INTO data_table(id, data)
VALUES
(1, '{"string_field":"from json", "integer_field":42, "int64_filed" : 1099511627776, "float": 3.14, "nested":{"ns":"boo"}}');

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
