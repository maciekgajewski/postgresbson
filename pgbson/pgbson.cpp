// mongo
#include "mongo/db/jsobj.h"
#include "mongo/db/json.h"
#ifdef LOG
#undef LOG
#endif

//postgres
extern "C" {
#include "postgres.h"
#include "fmgr.h"
#include "executor/executor.h"
#include "utils/typcache.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "parser/parse_coerce.h"
#include "catalog/pg_type.h"

// bson access macros
#define DatumGetBson(X) ((bytea *) PG_DETOAST_DATUM_PACKED(X))
#define GETARG_BSON(n)  DatumGetBson(PG_GETARG_DATUM(n))

}

#include <string>
#include <cstring>

// util
static Datum return_string(const std::string& s)
{
    std::size_t text_size = s.length() + VARHDRSZ;
    text* new_text = (text *) palloc(text_size);
    SET_VARSIZE(new_text, text_size);
    std::memcpy(VARDATA(new_text), s.c_str(), s.length());
    PG_RETURN_TEXT_P(new_text);
}

static Datum return_cstring(const std::string& s)
{
    char* c = (char*) palloc(s.length()+1);
    std::memcpy(c, s.c_str(), s.length()+1); // +1 to copy the null terminator as well

    PG_RETURN_CSTRING(c);
}

static Datum return_bson(const mongo::BSONObj& b)
{
    std::size_t bson_size = b.objsize() + VARHDRSZ;
    bytea* new_bytea = (bytea *) palloc(bson_size);
    SET_VARSIZE(new_bytea, bson_size);
    std::memcpy(VARDATA(new_bytea), b.objdata(), b.objsize());
    PG_RETURN_BYTEA_P(new_bytea);
}
// object inspection
template<typename FieldType>
static
Datum convert_element(PG_FUNCTION_ARGS, const mongo::BSONElement e);

template<typename FieldType>
static
Datum bson_get(PG_FUNCTION_ARGS)
{
    bytea* arg = GETARG_BSON(0);
    mongo::BSONObj object(VARDATA_ANY(arg));

    text* arg2 = PG_GETARG_TEXT_P(1);

    mongo::BSONElement e = object.getFieldDotted(VARDATA(arg2));
    if (e.eoo())
    {
        // no such element
        PG_RETURN_NULL();
    }
    else
    {
        return convert_element<FieldType>(fcinfo, e);
    }
}

template<>
Datum
convert_element<std::string>(PG_FUNCTION_ARGS, const mongo::BSONElement e)
{
    try
    {
        return return_string(e.String());
    }
    catch(...)
    {
        PG_RETURN_NULL();
    }
}
static
std::string
get_typename(Oid typid)
{
    HeapTuple	tp;

    tp = SearchSysCache1(TYPEOID, ObjectIdGetDatum(typid));
    if (HeapTupleIsValid(tp))
    {
        Form_pg_type typtup = (Form_pg_type) GETSTRUCT(tp);

        NameData result = typtup->typname;
        ReleaseSysCache(tp);
        return std::string(NameStr(result));
    }
    else
        return std::string();
}

static
void
composite_to_bson(mongo::BSONObjBuilder& builder, Datum composite);

static
void
datum_to_bson(const char* field_name, mongo::BSONObjBuilder& builder,
    Datum val, bool is_null, Oid typid)
{
    std::cout << "BEGIN datum_to_bson, field_name=" << field_name << ", typeid=" << typid << std::endl;

    if (field_name == NULL)
    {
        field_name = "";
    }

    if (is_null)
    {
        builder.appendNull(field_name);
    }
    else
    {
        switch(typid)
        {
            case BOOLOID:
                 builder.append(field_name, DatumGetBool(val));
                 break;

            case CHAROID:
            {
                char c = DatumGetChar(val);
                builder.append(field_name, &c, 1);
                break;
            }

            case INT8OID:
                builder.append(field_name, (long long)DatumGetInt64(val));
                break;

            case INT2OID:
                builder.append(field_name, DatumGetInt16(val));
                break;

            case INT4OID:
                builder.append(field_name, DatumGetInt32(val));
                break;

            case TEXTOID:
            case JSONOID:
            case XMLOID:
            {
                text* t = DatumGetTextP(val);
                builder.append(field_name, VARDATA(t), VARSIZE(t)-VARHDRSZ+1);
                break;
            }

            case FLOAT4OID:
                builder.append(field_name, DatumGetFloat4(val));
                break;

            case FLOAT8OID:
                builder.append(field_name, DatumGetFloat8(val));
                break;

            case RECORDOID:
            {
                mongo::BSONObjBuilder sub(builder.subobjStart(field_name));
                composite_to_bson(sub, val);
                sub.done();
                break;
            }

            case TIMESTAMPOID:
            {
                Timestamp ts = DatumGetTimestamp(val);
                #ifdef HAVE_INT64_TIMESTAMP
                mongo::Date_t date(ts);
                #else
                mongo::Date_t date(ts * 1000);
                #endif

                builder.append(field_name, date);
                break;
            }

            default:
            {
                std::cout << "datum_to_bson - unknown type, using text output." << std::endl;
                std::cout << "datum_to_bson - type=" << get_typename(typid) << std::endl;
                if (get_typename(typid) == "bson")
                {
                    bytea* data = DatumGetByteaPP(val);
                    mongo::BSONObj obj(VARDATA(data));
                    builder.append(field_name, obj);
                }
                else
                {
                    // use text output for the type
                    bool typisvarlena = false;
                    Oid typoutput;
                    getTypeOutputInfo(typid, &typoutput, &typisvarlena);
                    std::cout << "datum_to_bson - typisvarlena=" << std::boolalpha << typisvarlena << std::endl;
                    Datum out_val = val;
                    /*
                     * If we have a toasted datum, forcibly detoast it here to avoid
                     * memory leakage inside the type's output routine.
                     */
                    if (typisvarlena)
                    {
                        out_val = PointerGetDatum(PG_DETOAST_DATUM(val));
                        std::cout << "datum_to_bson - var len valuie detoasted" << std::endl;
                    }

                    char* outstr = OidOutputFunctionCall(typoutput, out_val);
                    builder.append(field_name, outstr);

                    /* Clean up detoasted copy, if any */
                    if (val != out_val)
                        pfree(DatumGetPointer(out_val));
                }
            }
        } // switch
    } // if not null

    std::cout << "END datum_to_bson, field_name=" << field_name << std::endl;

}



static
void
composite_to_bson(mongo::BSONObjBuilder& builder, Datum composite)
{
    std::cout << "BEGIN composite_to_bson" << std::endl;

    HeapTupleHeader td;
    Oid			tupType;
    int32		tupTypmod;
    TupleDesc	tupdesc;
    HeapTupleData tmptup;

    td = DatumGetHeapTupleHeader(composite);

    /* Extract rowtype info and find a tupdesc */
    tupType = HeapTupleHeaderGetTypeId(td);
    tupTypmod = HeapTupleHeaderGetTypMod(td);
    tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

    /* Build a temporary HeapTuple control structure */
    tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
    tmptup.t_data = td;
    HeapTupleData* tuple = &tmptup;

    for (int i = 0; i < tupdesc->natts; i++)
    {
        bool		isnull;

        if (tupdesc->attrs[i]->attisdropped)
            continue;

        const char* field_name = NameStr(tupdesc->attrs[i]->attname);
        Datum val = heap_getattr(tuple, i + 1, tupdesc, &isnull);
        datum_to_bson(field_name, builder, val, isnull, tupdesc->attrs[i]->atttypid);

    }

    ReleaseTupleDesc(tupdesc);
    std::cout << "END composite_to_bson" << std::endl;
}

extern "C" {

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// dummy test function
PG_FUNCTION_INFO_V1(pgbson_test);
Datum pgbson_test(PG_FUNCTION_ARGS)
{
    return return_string("OK");
}

// bson output - to json
PG_FUNCTION_INFO_V1(bson_out);
Datum
bson_out(PG_FUNCTION_ARGS)
{
    bytea* arg = GETARG_BSON(0);
    mongo::BSONObj object(VARDATA_ANY(arg));

    std::string json = object.jsonString(); // strict, not-pretty
    return return_cstring(json);
}

// bson input - from json
PG_FUNCTION_INFO_V1(bson_in);
Datum
bson_in(PG_FUNCTION_ARGS)
{
    char* arg = PG_GETARG_CSTRING(0);
    try
    {
        mongo::BSONObj object = mongo::fromjson(arg, NULL);
        // copy to palloc-ed buffer
        return return_bson(object);
    }
    catch(...)
    {
        ereport(
            ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), errmsg("invalid input syntax for BSON"))
        );
    }
}

PG_FUNCTION_INFO_V1(bson_get_string);
Datum
bson_get_string(PG_FUNCTION_ARGS)
{
    return bson_get<std::string>(fcinfo);
}

// Converts composite type to BSON
//
// Code of this function is based on row_to_json
PG_FUNCTION_INFO_V1(row_to_bson);
Datum
row_to_bson(PG_FUNCTION_ARGS)
{
    std::cout << "row_to_bson" << std::endl;
    Datum record = PG_GETARG_DATUM(0);
    mongo::BSONObjBuilder builder;

    composite_to_bson(builder, record);

    return return_bson(builder.obj());
}


}
