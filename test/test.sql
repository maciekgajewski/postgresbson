\qecho * loading extension
CREATE EXTENSION pgbson;

\qecho * creating table
CREATE TEMPORARY TABLE data_table (
    id BIGSERIAL,
    data BSON
);

\qecho * json format input
INSERT INTO data_table(id, data)
VALUES
(1, '{"string_field":"from json", "integer_field":42, "nested":{"ns":"boo"}}');

\qecho * bson from row

CREATE TYPE nested_obj_type AS (ns TEXT);
CREATE TYPE obj_type AS (string_fiels TEXT, integer_field INTEGER, nested BSON);


INSERT INTO data_table(id, data)
VALUES
(1, row_to_bson(
    row(
        'from_json',
        42,
        row_to_bson(row('boo')::nested_obj_type)
        )::obj_type
    )
);

