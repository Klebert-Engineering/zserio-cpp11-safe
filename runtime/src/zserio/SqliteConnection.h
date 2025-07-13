#ifndef ZSERIO_SQL_CONNECTION_H_INC
#define ZSERIO_SQL_CONNECTION_H_INC

#include <memory>

#include "zserio/Result.h"
#include "zserio/SqliteFinalizer.h"
#include "zserio/StringView.h"

#include "sqlite3.h"

namespace zserio
{

/**
 * Helper class to keep sqlite3 connection and ensure its safe destruction.
 *
 * The class also provides simple interface to execute SQLite queries.
 */
class SqliteConnection
{
public:
    /**
     * Connection type.
     */
    enum ConnectionType
    {
        INTERNAL_CONNECTION, /**< Internal connection which must be released in destructor. */
        EXTERNAL_CONNECTION /**< External connection managed from outside. */
    };

    /**
     * Constructor.
     *
     * \param connection Pointer to the SQLite connection.
     * \param connectionType Type of the connection. Default is INTERNAL_CONNECTION.
     */
    explicit SqliteConnection(
            sqlite3* connection = nullptr, ConnectionType connectionType = INTERNAL_CONNECTION);

    /**
     * Destructor.
     */
    ~SqliteConnection();

    /**
     * Copying and moving is disallowed!
     * \{
     */
    SqliteConnection(const SqliteConnection&) = delete;
    SqliteConnection& operator=(const SqliteConnection&) = delete;

    SqliteConnection(SqliteConnection&&) = delete;
    SqliteConnection& operator=(SqliteConnection&&) = delete;
    /** \} */

    /**
     * Resets the connection.
     *
     * \param connection New connection to set. Default is NULL - i.e. unset.
     * \param connectionType Type of the new connection.
     */
    void reset(sqlite3* connection = nullptr, ConnectionType connectionType = INTERNAL_CONNECTION);

    /**
     * Gets the current connection type.
     *
     * When connection is NULL, the connection type is insignificant.
     *
     * \return Connection type.
     */
    ConnectionType getConnectionType() const;

    /**
     * Gets the current connection.
     *
     * \return SQLite connection.
     */
    sqlite3* getConnection();

    /**
     * Executes a single query which doesn't need to return anything - e.g. DML.
     *
     * \param sqlQuery The query string.
     *
     * \return Success or error code on failure.
     */
    Result<void> executeUpdate(StringView sqlQuery) noexcept;

    /**
     * Prepares the SQLite statement for the given query.
     *
     * Note that the user is responsible to proper statement finalization using sqlite3_finalize!
     *
     * \param sqlQuery The query string.
     *
     * \return Result containing prepared SQLite statement or error code on failure.
     */
    Result<sqlite3_stmt*> prepareStatement(StringView sqlQuery) noexcept;

    /**
     * Starts a new transaction if a transaction is not already started.
     *
     * \return Result containing true when the new transaction was started, false when a transaction is already started, or error code on failure.
     */
    Result<bool> startTransaction() noexcept;

    /**
     * Terminates the current transaction.
     *
     * The parameter wasTransactionStarted is used for convenience since it's then easier to write code
     * which uses transactions.
     *
     * \code{.cpp}
     * auto result = connection.startTransaction(); // transaction may be already started
     * if (result.isError()) return result.getError();
     * bool wasTransactionStarted = result.getValue();
     * // execute queries
     * // ...
     * // terminates the transaction only if it was started by the corresponding startTransaction call.
     * auto endResult = connection.endTransaction(wasTransactionStarted);
     * if (endResult.isError()) return endResult.getError();
     * \endcode
     *
     * \param wasTransactionStarted When false, the call does actually nothing.
     *
     * \return Success or error code on failure.
     */
    Result<void> endTransaction(bool wasTransactionStarted) noexcept;

private:
    sqlite3* m_connection;
    ConnectionType m_connectionType;
};

inline SqliteConnection::SqliteConnection(sqlite3* connection, ConnectionType connectionType) :
        m_connection(connection),
        m_connectionType(connectionType)
{}

inline SqliteConnection::~SqliteConnection()
{
    reset();
}

inline void SqliteConnection::reset(sqlite3* connection, ConnectionType connectionType)
{
    // close connection only if it is internal
    if (m_connectionType == INTERNAL_CONNECTION)
    {
        sqlite3_close_v2(m_connection); // sqlite3_close_v2(NULL) is a harmless no-op
    }

    m_connection = connection;
    m_connectionType = connectionType;
}

inline SqliteConnection::ConnectionType SqliteConnection::getConnectionType() const
{
    return m_connectionType;
}

inline sqlite3* SqliteConnection::getConnection()
{
    return m_connection;
}

inline Result<void> SqliteConnection::executeUpdate(StringView sqlQuery) noexcept
{
    auto statementResult = prepareStatement(sqlQuery);
    if (statementResult.isError())
    {
        return Result<void>::error(statementResult.getError());
    }
    
    std::unique_ptr<sqlite3_stmt, SqliteFinalizer> statement(statementResult.getValue());
    int result = sqlite3_step(statement.get());
    if (result != SQLITE_DONE)
    {
        return Result<void>::error(ErrorCode::SqliteError);
    }
    
    return Result<void>::success();
}

inline Result<sqlite3_stmt*> SqliteConnection::prepareStatement(StringView sqlQuery) noexcept
{
    sqlite3_stmt* statement = nullptr;
    const int result = sqlite3_prepare_v2(
            m_connection, sqlQuery.data(), static_cast<int>(sqlQuery.size()), &statement, nullptr);
    if (result != SQLITE_OK)
    {
        return Result<sqlite3_stmt*>::error(ErrorCode::SqliteError);
    }

    return Result<sqlite3_stmt*>::success(statement);
}

inline Result<bool> SqliteConnection::startTransaction() noexcept
{
    bool wasTransactionStarted = false;
    if (sqlite3_get_autocommit(m_connection) != 0)
    {
        auto result = executeUpdate("BEGIN;");
        if (result.isError())
        {
            return Result<bool>::error(result.getError());
        }
        wasTransactionStarted = true;
    }

    return Result<bool>::success(wasTransactionStarted);
}

inline Result<void> SqliteConnection::endTransaction(bool wasTransactionStarted) noexcept
{
    if (wasTransactionStarted)
    {
        return executeUpdate("COMMIT;");
    }
    return Result<void>::success();
}

} // namespace zserio

#endif // ZSERIO_SQL_CONNECTION_H_INC
