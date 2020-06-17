/**
 * SQLite connection core - low-level SQLite connection C library API
 */

#include "sqlite-connection-core.h"

#include <stdbool.h>

#include <stddef.h>

#include <math.h>

#include <string.h>

#include "sqlite3.h"

// **Ugly hack** to make Xcode happy for iOS:
#ifdef SCC_INCLUDE_FAKE_CRYPTO_KEY
#include "sqlite-fake-crypto-key.h"
#endif

#define DEFAULT_MAX_SCC_CONNECTIONS 1000

#ifdef SCC_MAXIMUM_CONNECTIONS
#define MAX_SCC_RECORD_COUNT SCC_MAXIMUM_CONNECTIONS
#else
#define MAX_SCC_RECORD_COUNT DEFAULT_MAX_SCC_CONNECTIONS
#endif

#define FIRST_SCC_RECORD_ID 1

bool is_initialized = false;

// use sqlite3_mutex ref:
// https://www.sqlite.org/c3ref/mutex_alloc.html
sqlite3_mutex * _open_mutex = NULL;

#define START_OPEN_MUTEX()  do {    \
  sqlite3_mutex_enter(_open_mutex); \
} while (false)

#define END_OPEN_MUTEX()    do {    \
  sqlite3_mutex_leave(_open_mutex); \
} while (false)

// General SCC record comment(s):
// It is not expected for multiple threads to require access
// from the *same* SCC record in parallel.
// Multi-threaded access should be done from different SCC connections.
// Keeping the _st_mutex lock for an entire read or write option
// is not expected to cause any potential issues.

typedef struct scc_record_s {
  sqlite3 * db;
  sqlite3_mutex * _st_mutex;
  sqlite3_stmt * _st;
} * scc_record_ref;

#define START_REC_ST_MUTEX(r)   do {    \
  sqlite3_mutex_enter(r->_st_mutex);    \
} while (false)

#define END_REC_ST_MUTEX(r) do {        \
  sqlite3_mutex_leave(r->_st_mutex);    \
} while (false)

static struct scc_record_s scc_record_list[MAX_SCC_RECORD_COUNT] = { NULL };

static int _scc_record_count = FIRST_SCC_RECORD_ID;

/**
 * NOT expected to be thread-safe, should be run in the main thread
 */
void scc_init()
{
  if (is_initialized) return;

  _open_mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_RECURSIVE);
  is_initialized = true;
}

int scc_open_connection(const char * filename, int flags)
{
  int connection_id = -1;
  sqlite3 * db = NULL;
  int open_result = SQLITE_ERROR;

  if (!is_initialized) {
    // BOGUS
    return -1;
  }

  START_OPEN_MUTEX();

  if (_scc_record_count < MAX_SCC_RECORD_COUNT) {
    connection_id = _scc_record_count;
    ++_scc_record_count;
  }

  if (connection_id != -1) {
    open_result = sqlite3_open_v2(filename, &db, flags, NULL);

#ifndef NO_SCC_DBCONFIG_DEFENSIVE
    // extra-safe ref:
    // - https://www.sqlite.org/c3ref/c_dbconfig_defensive.html#sqlitedbconfigdefensive
    // - https://www.sqlite.org/releaselog/3_26_0.html
    // - https://github.com/xpbrew/cordova-sqlite-storage-help/issues/34#issuecomment-597821628
    // with a 4th argument needed ref:
    // - http://sqlite.1065341.n5.nabble.com/sqlite3-db-config-documentation-issue-td106755.html
    if (open_result == SQLITE_OK) {
      open_result = sqlite3_db_config(db, SQLITE_DBCONFIG_DEFENSIVE, 1, NULL);

      // just in case:
      if (open_result != SQLITE_OK) {
        sqlite3_close(db);
      }
    }
#endif

    if (open_result != SQLITE_OK) {
      // release the internal resource:
      --_scc_record_count;
    }
  }

  END_OPEN_MUTEX();

  if (open_result == SQLITE_OK) {
    scc_record_list[connection_id].db = db;
    scc_record_list[connection_id]._st_mutex =
      sqlite3_mutex_alloc(SQLITE_MUTEX_RECURSIVE);
    scc_record_list[connection_id]._st = NULL;
  }

  return (open_result == SQLITE_OK) ? connection_id : -open_result;
}

int scc_key(int connection_id, const char * key)
{
#ifdef NO_SCC_CRYPTO_KEY
  // SIGNAL ERROR:
  return -1;
#else
  if (connection_id < 0) {
    // BOGUS
    return 0;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    return sqlite3_key(r->db, key, strlen(key));
  }
#endif
}

int scc_begin_statement(int connection_id, const char * statement)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st != NULL) {
      rc = 21; // SQLite abuse
    } else {
      sqlite3_stmt * st;
      rc = sqlite3_prepare_v2(r->db, statement, -1, &st, NULL);
      r->_st = st;
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_bind_text(int connection_id, int index, const char * text)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_bind_text(st, index, text, -1, SQLITE_TRANSIENT);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_bind_text_utf8_bytes(int connection_id, int index, void * text, int length)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_bind_text(st, index, text, length, SQLITE_TRANSIENT);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_bind_text_utf16_bytes(int connection_id, int index, void * text, int length)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_bind_text16(st, index, text, length, SQLITE_TRANSIENT);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_bind_double(int connection_id, int index, double value)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_bind_double(st, index, value);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_bind_long(int connection_id, int index, scc_long_long value)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_bind_int64(st, index, value);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_bind_null(int connection_id, int index)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_bind_null(st, index);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_step(int connection_id)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_step(st);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

const char * scc_get_last_error_message(int connection_id)
{
  if (connection_id < 0) {
    // BOGUS
    return "invalid connection id";
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    return sqlite3_errmsg(r->db);
  }
}

int scc_get_column_count(int connection_id)
{
  if (connection_id < 0) {
    // BOGUS
    return -1;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      // BOGUS
      rc = -1;
    } else {
      rc = sqlite3_column_count(st);
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

const char * scc_get_column_name(int connection_id, int column)
{
  if (connection_id < 0) {
    // BOGUS
    return NULL;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    const char * value;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      value = "";
    } else {
      value = sqlite3_column_name(st, column);
    }
    END_REC_ST_MUTEX(r);
    return value;
  }
}

int scc_get_column_type(int connection_id, int column)
{
  if (connection_id < 0) {
    // BOGUS
    return -1;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int value;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      // BOGUS
      value = -1;
    } else {
      value = sqlite3_column_type(st, column);
    }
    END_REC_ST_MUTEX(r);
    return value;
  }
}

const char * scc_get_column_text(int connection_id, int column)
{
  if (connection_id < 0) {
    // BOGUS
    return NULL;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    const char * value;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      value = NULL;
    } else {
      value = (const char *)sqlite3_column_text(st, column);
      // TBD quick workaround solution to avoid a possible crash:
      if (value == NULL) value = "";
    }
    END_REC_ST_MUTEX(r);
    return value;
  }
}

double scc_get_column_double(int connection_id, int column)
{
  if (connection_id < 0) {
    // BOGUS
    return NAN;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    double value;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      // BOGUS
      value = NAN;
    } else {
      value = sqlite3_column_double(st, column);
    }
    END_REC_ST_MUTEX(r);
    return value;
  }
}

scc_long_long scc_get_column_long(int connection_id, int column)
{
  if (connection_id < 0) {
    // BOGUS
    return 0;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    scc_long_long value;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      // BOGUS
      value = 0;
    } else {
      value = sqlite3_column_int64(st, column);
    }
    END_REC_ST_MUTEX(r);
    return value;
  }
}

int scc_end_statement(int connection_id)
{
  if (connection_id < 0) {
    return 21; // SQLite abuse
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    sqlite3_stmt * st;
    int rc;

    START_REC_ST_MUTEX(r);
    st = r->_st;
    if (st == NULL) {
      rc = 21; // SQLite abuse
    } else {
      rc = sqlite3_finalize(st);
      r->_st = NULL;
    }
    END_REC_ST_MUTEX(r);
    return rc;
  }
}

int scc_get_total_changes(int connection_id)
{
  if (connection_id < 0) {
    // BOGUS
    return 0;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    return sqlite3_total_changes(r->db);
  }
}

int scc_get_last_insert_rowid(int connection_id)
{
  if (connection_id < 0) {
    // BOGUS
    return 0;
  } else {
    scc_record_ref r = &scc_record_list[connection_id];
    return sqlite3_last_insert_rowid(r->db);
  }
}
