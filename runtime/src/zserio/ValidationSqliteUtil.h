#ifndef ZSERIO_VALIDATION_SQLITE_UTIL_H_INC
#define ZSERIO_VALIDATION_SQLITE_UTIL_H_INC

#include <map>

#include "zserio/RebindAlloc.h"
#include "zserio/Result.h"
#include "zserio/SqliteConnection.h"
#include "zserio/SqliteFinalizer.h"
#include "zserio/String.h"
#include "zserio/StringView.h"

namespace zserio
{

/** Sqlite utility for validation. */
template <typename ALLOC>
struct ValidationSqliteUtil
{
    using string_type = string<ALLOC>;
    using Statement = std::unique_ptr<sqlite3_stmt, SqliteFinalizer>;

    /**
     * Description of a single column.
     */
    struct ColumnDescription
    {
        string_type name; /**< Column name. */
        string_type type; /**< Column SQLite data type ("INTEGER", "REAL", "TEXT" or "BLOB"). */
        bool isNotNull; /**< Is true if the column has "NOT NULL" constraint. */
        bool isPrimaryKey; /**< Is true if the column is primary key. */
    };

    using TableSchema = std::map<string_type, ColumnDescription, std::less<string_type>,
            RebindAlloc<ALLOC, std::pair<const string_type, ColumnDescription>>>;

    /**
     * Gets number of rows in the given SQLite table.
     *
     * \param connection     Database connection to use.
     * \param attachedDbName Attached database name if table is relocated in different database.
     * \param tableName      Name of the table to get column types of.
     * \param allocator      Allocator to use for the query string composition.
     *
     * \return Result containing number of rows or error code on failure.
     */
    static Result<size_t> getNumberOfTableRows(SqliteConnection& connection, StringView attachedDbName,
            StringView tableName, const ALLOC& allocator) noexcept
    {
        string_type sqlQuery(allocator);
        sqlQuery += "SELECT count(*) FROM ";
        if (!attachedDbName.empty())
        {
            sqlQuery += attachedDbName;
            sqlQuery += ".";
        }
        sqlQuery += tableName;

        auto statementResult = connection.prepareStatement(sqlQuery);
        if (statementResult.isError())
        {
            return Result<size_t>::error(statementResult.getError());
        }
        
        Statement statement(statementResult.getValue());
        const int result = sqlite3_step(statement.get());
        if (result != SQLITE_ROW)
        {
            return Result<size_t>::error(ErrorCode::SqliteError);
        }

        return Result<size_t>::success(static_cast<size_t>(sqlite3_column_int64(statement.get(), 0)));
    }

    /**
     * Gets a map of column names to column description for given SQLite table.
     *
     * \param connection     Database connection to use.
     * \param attachedDbName Attached database name if table is relocated in different database.
     * \param tableName      Name of the table to get column types of.
     * \param tableSchema    Schema to fill.
     * \param allocator      Allocator to use for the query string composition.
     *
     * \return Success or error code on failure.
     */
    static Result<void> getTableSchema(SqliteConnection& connection, StringView attachedDbName, StringView tableName,
            TableSchema& tableSchema, const ALLOC& allocator) noexcept
    {
        string_type sqlQuery(allocator);
        sqlQuery += "PRAGMA ";
        if (!attachedDbName.empty())
        {
            sqlQuery += attachedDbName;
            sqlQuery += ".";
        }
        sqlQuery += "table_info(";
        sqlQuery += tableName;
        sqlQuery += ")";

        auto statementResult = connection.prepareStatement(sqlQuery);
        if (statementResult.isError())
        {
            return Result<void>::error(statementResult.getError());
        }
        
        Statement statement(statementResult.getValue());

        int result = SQLITE_OK;
        while ((result = sqlite3_step(statement.get())) == SQLITE_ROW)
        {
            const char* columnName = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1));
            const char* columnType = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 2));
            tableSchema.emplace(string_type(columnName, allocator),
                    ColumnDescription{
                            string_type(columnName, allocator), string_type(columnType, allocator),
                            sqlite3_column_int(statement.get(), 3) != 0, // is not null
                            sqlite3_column_int(statement.get(), 5) != 0 // is primary key
                    });
        }

        if (result != SQLITE_DONE)
        {
            return Result<void>::error(ErrorCode::SqliteError);
        }
        
        return Result<void>::success();
    }

    /**
     * Check if the column is present in the given SQLite table. Note that this method also detect
     * hidden SQLite columns, which are not visible using standard PRAGMA table_info query.
     *
     * \param connection     Database connection to use.
     * \param attachedDbName Attached database name if table is relocated in different database.
     * \param tableName      Name of the table to get column types of.
     * \param columnName     Name of the column to check.
     * \param allocator      Allocator to use for the query string composition.
     *
     * \return Result containing true if the column is present in the table (even if hidden), false otherwise, or error code on failure.
     */
    static Result<bool> isColumnInTable(SqliteConnection& connection, StringView attachedDbName, StringView tableName,
            StringView columnName, const ALLOC& allocator) noexcept
    {
        // try select to check if hidden column exists
        string_type sqlQuery(allocator);
        sqlQuery += "SELECT ";
        sqlQuery += columnName;
        sqlQuery += " FROM ";
        if (!attachedDbName.empty())
        {
            sqlQuery += attachedDbName;
            sqlQuery += ".";
        }
        sqlQuery += tableName;
        sqlQuery += " LIMIT 0";

        auto statementResult = connection.prepareStatement(sqlQuery);
        if (statementResult.isError())
        {
            // If prepare fails, column doesn't exist
            return Result<bool>::success(false);
        }
        
        Statement statement(statementResult.getValue());
        return Result<bool>::success(sqlite3_step(statement.get()) == SQLITE_DONE);
    }

    /**
     * Gets name of the given SQLite column type.
     *
     * \param columnType SQLite column type.
     *
     * \return SQLite column type name.
     */
    static const char* sqliteColumnTypeName(int columnType)
    {
        switch (columnType)
        {
        case SQLITE_INTEGER:
            return "INTEGER";
        case SQLITE_FLOAT:
            return "REAL";
        case SQLITE_TEXT:
            return "TEXT";
        case SQLITE_BLOB:
            return "BLOB";
        default:
            return "NULL";
        }
    }
};

} // namespace zserio

#endif // ZSERIO_VALIDATION_SQLITE_UTIL_H_INC
