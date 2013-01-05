\qecho * loading extension
CREATE EXTENSION pgbson;

\qecho * creating tables
CREATE TEMPORARY TABLE data_table (
    id BIGSERIAL,
    data BSON
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
(1, '{"string_field":"from json", "integer_field":42, "nested":{"ns":"boo"}}');

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
SELECT 'bson_get_as_bson on string',
    '{"":"from json"}'::bson::text, bson_get_as_bson(data, 'string_field')::text  FROM data_table WHERE id = 1;

INSERT INTO results_table(name, expected, got)
SELECT 'bson_get_as_bson on nested object',
    '{"ns":"boo"}'::bson::text, bson_get_as_bson(data, 'nested')::text  FROM data_table WHERE id = 1;

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

-- this must be at the end
\qecho * test results, check for failures!
SELECT *, expected = got AS passed FROM results_table;
