/**
 * SQLite connection core - low-level SQLite connection C library API
 */

// ref: https://www.sqlite.org/c3ref/c_blob.html
#define SCC_COLUMN_TYPE_INTEGER 1
#define SCC_COLUMN_TYPE_FLOAT 2
#define SCC_COLUMN_TYPE_TEXT 3
#define SCC_COLUMN_TYPE_NULL 5

/**
 * This typedef is needed to help gluegen generate JNI C code
 * that is using the correct data type.
 */
typedef long long scc_long_long;

/**
 * This required initialization function should be called from the
 * main thread upon startup, is __NOT__ thread-safe.
 */
void scc_init();

int scc_open_connection(const char * filename, int flags);

int scc_key(int connection_id, const char * key);

int scc_begin_statement(int connection_id, const char * statement);

int scc_bind_text(int connection_id, int index, const char * text);

int scc_bind_text_utf8_bytes(int connection_id, int index, void * text, int length);

int scc_bind_text_utf16_bytes(int connection_id, int index, void * text, int length);

int scc_bind_double(int connection_id, int index, double value);

int scc_bind_long(int connection_id, int index, scc_long_long value);

int scc_bind_null(int connection_id, int index);

int scc_step(int connection_id);

const char * scc_get_last_error_message(int connection_id);

int scc_get_column_count(int connection_id);

const char * scc_get_column_name(int connection_id, int column);

int scc_get_column_type(int connection_id, int column);

const char * scc_get_column_text(int connection_id, int column);

double scc_get_column_double(int connection_id, int column);

scc_long_long scc_get_column_long(int connection_id, int column);

int scc_get_total_changes(int connection_id);

int scc_get_last_insert_rowid(int connection_id);

int scc_end_statement(int connection_id);
