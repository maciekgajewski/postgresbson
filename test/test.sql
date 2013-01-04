\qecho * loading extension
CREATE EXTENSION pgbson;

\qecho * creating tables
CREATE TEMPORARY TABLE data_table (
    id BIGSERIAL,
    data BSON
);

CREATE TEMPORARY TABLE test_resulsts (
    name TEXT NOT NULL,
    expected TEXT,
    got TEXT
);

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


INSERT INTO test_resulsts(name, expected, got)
SELECT 'bson_get_string on bson from json',
    'from json', bson_get_text(data, 'string_field') FROM data_table WHERE id = 1;

INSERT INTO test_resulsts(name, expected, got)
SELECT 'bson_get_string on bson from row',
    'from row', bson_get_text(data, 'string_field')  FROM data_table WHERE id = 2;





-- this must be at the end
\qecho * Test results, check for failures
SELECT *, expected = got AS passed FROM test_resulsts;
