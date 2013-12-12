// Copyright (c) 2012-2013 Maciej Gajewski <maciej.gajewski0@gmail.com>
// 
// Permission to use, copy, modify, and distribute this software and its documentation for any purpose, without fee, and without a written agreement is hereby granted,
// provided that the above copyright notice and this paragraph and the following two paragraphs appear in all copies.
// 
// IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, 
// ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE AUTHOR HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

#include "pgbson_internal.hpp"

extern "C" {
#include <utils/numeric.h>
#include <access/tuptoaster.h>
}

Datum return_string(const std::string& s)
{
    std::size_t text_size = s.length() + VARHDRSZ;
    text* new_text = (text *) palloc(text_size);
    SET_VARSIZE(new_text, text_size);
    std::memcpy(VARDATA(new_text), s.c_str(), s.length());

    PG_RETURN_TEXT_P(new_text);
}

Datum return_cstring(const std::string& s)
{
    char* c = (char*) palloc(s.length()+1);
    std::memcpy(c, s.c_str(), s.length()+1); // +1 to copy the null terminator as well

    PG_RETURN_CSTRING(c);
}

Datum return_bson(const mongo::BSONObj& b)
{
    std::size_t bson_size = b.objsize() + VARHDRSZ;
    bytea* new_bytea = (bytea *) palloc(bson_size);
    SET_VARSIZE(new_bytea, bson_size);
    std::memcpy(VARDATA(new_bytea), b.objdata(), b.objsize());
    PG_RETURN_BYTEA_P(new_bytea);
}

std::string get_typename(Oid typid)
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


void composite_to_bson(mongo::BSONObjBuilder& builder, Datum composite)
{
    PGBSON_LOG << "BEGIN composite_to_bson" << PGBSON_ENDL;

    HeapTupleHeader td;
    Oid         tupType;
    int32       tupTypmod;
    TupleDesc   tupdesc;
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
        bool isnull;

        if (tupdesc->attrs[i]->attisdropped)
            continue;

        const char* field_name = NameStr(tupdesc->attrs[i]->attname);
        Datum val = heap_getattr(tuple, i + 1, tupdesc, &isnull);
        datum_to_bson(field_name, builder, val, isnull, tupdesc->attrs[i]->atttypid);

    }

    ReleaseTupleDesc(tupdesc);
    PGBSON_LOG << "END composite_to_bson" << PGBSON_ENDL;
}

void datum_to_bson(const char* field_name, mongo::BSONObjBuilder& builder,
    Datum val, bool is_null, Oid typid)
{
    PGBSON_LOG << "BEGIN datum_to_bson, field_name=" << field_name << ", typeid=" << typid << PGBSON_ENDL;

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
                PGBSON_LOG << "datum_to_bson - unknown type, using text output." << PGBSON_ENDL;
                PGBSON_LOG << "datum_to_bson - type=" << get_typename(typid) << PGBSON_ENDL;
                if (get_typename(typid) == "bson")
                {
                    bytea* data = DatumGetBson(val);
                    mongo::BSONObj obj(VARDATA_ANY(data));
                    builder.append(field_name, obj);
                }
                else
                {
                    // use text output for the type
                    bool typisvarlena = false;
                    Oid typoutput;
                    getTypeOutputInfo(typid, &typoutput, &typisvarlena);
                    PGBSON_LOG << "datum_to_bson - typisvarlena=" << std::boolalpha << typisvarlena << PGBSON_ENDL;
                    Datum out_val = val;
                    /*
                     * If we have a toasted datum, forcibly detoast it here to avoid
                     * memory leakage inside the type's output routine.
                     */
                    if (typisvarlena)
                    {
                        out_val = PointerGetDatum(PG_DETOAST_DATUM(val));
                        PGBSON_LOG << "datum_to_bson - var len valuie detoasted" << PGBSON_ENDL;
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

    PGBSON_LOG << "END datum_to_bson, field_name=" << field_name << PGBSON_ENDL;

}

template<>
Datum convert_element<std::string>(PG_FUNCTION_ARGS, const mongo::BSONElement e)
{
    std::stringstream ss;
    switch(e.type())
    {
        case mongo::String:
        case mongo::DBRef:
        case mongo::Symbol:
            return return_string(std::string(e.valuestr(), e.valuestrsize()-1));

        case mongo::NumberDouble:
            ss << e._numberDouble();
            break;

        case mongo::jstOID:
            ss << e.__oid().str();
            break;

        case mongo::Bool:
            ss << std::boolalpha << e.boolean();
            break;

        case mongo::Date:
            return return_string(
                to_iso_extended_string(
                    boost::posix_time::ptime(
                        boost::gregorian::date(1970, 1, 1),
                        boost::posix_time::milliseconds(e.date().millis)
                        )
                    )
                );

        case mongo::RegEx:
            ss << e.regex();
            break;

        case mongo::NumberInt:
            ss << e._numberInt();
            break;

        case mongo::NumberLong:
            ss << e._numberLong();
            break;

        default:
            throw convertion_error("text");

    }
    return return_string(ss.str());
}

template<>
Datum convert_element<int>(PG_FUNCTION_ARGS, const mongo::BSONElement e)
{
    if(e.type() == mongo::NumberInt)
    {
        PG_RETURN_INT32(e._numberInt());
    }
    else
    {
        throw convertion_error("int4");
    }
}

template<>
Datum convert_element<double>(PG_FUNCTION_ARGS, const mongo::BSONElement e)
{
    switch(e.type())
    {
        case mongo::NumberDouble:
            PG_RETURN_FLOAT8(e._numberDouble());

        // this conversion has to be imlicitly allowedm, otherwise defining object form json would be pain int the ass
        case mongo::NumberInt:
            PG_RETURN_FLOAT8(static_cast<double>(e._numberInt()));

        default:
            throw convertion_error("float8");
    }
}

template<>
Datum convert_element<int64>(PG_FUNCTION_ARGS, const mongo::BSONElement e)
{
    switch(e.type())
    {
        case mongo::NumberLong:
            PG_RETURN_INT64(e._numberLong());

        // this conversion has to be imlicitly allowedm, otherwise defining object form json would be pain int the ass
        case mongo::NumberInt:
            PG_RETURN_INT64(static_cast<int64>(e._numberInt()));

        default:
            throw convertion_error("int8");
    }
}

const char* bson_type_name(const mongo::BSONElement& e)
{
    return mongo::typeName(e.type());
}

