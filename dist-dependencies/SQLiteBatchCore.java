package io.sqlc;

public class SQLiteBatchCore {
  public interface BatchData {
    int getEntryCount();
    void openEntry(int index);
    String getEntryStatement();
    int getEntryBindColumnCount();
    boolean isEntryBindColumnNumber(int column);
    boolean isEntryBindColumnString(int column);
    double getEntryBindColumnDouble(int column);
    String getEntryBindColumnString(int column);
  }

  public interface BatchResults {
    void startNewEntry();
    void entryPutFieldAsInteger(String name, int value);
    void entryPutFieldAsString(String name, String value);
    void putNewEntry();
    void startNewEntryWithRows();
    void entryPutColumnName(String name);
    void startEntryRow();
    void entryPutRowValueAsString(String value);
    void entryPutRowValueAsDouble(double value);
    void entryPutRowValueAsNull();
    void entryPutRow();
    void putNewEntryWithRows();
  }

  static public int openBatchConnection(String fullName, int flags) {
    return SCCoreGlue.scc_open_connection(fullName, flags);
  }

  public static void
  executeBatch(final int mydbc, BatchData batchData, BatchResults batchResults) {
    try {
      final int count = batchData.getEntryCount();

      for (int i=0; i<count; ++i) {
        int previousTotalChanges = SCCoreGlue.scc_get_total_changes(mydbc);

        batchData.openEntry(i);

        String statement = batchData.getEntryStatement();

        if (SCCoreGlue.scc_begin_statement(mydbc, statement) != 0) {
          batchResults.startNewEntry();
          batchResults.entryPutFieldAsInteger("status", 1); // SQLite ERROR 1
          batchResults.entryPutFieldAsString("message",
            SCCoreGlue.scc_get_last_error_message(mydbc));
          batchResults.putNewEntry();
        } else {
          final int bindCount = batchData.getEntryBindColumnCount();

          int bindResult = 0; // SQLite OK

          for (int j = 0; j < bindCount; ++j) {
            if (batchData.isEntryBindColumnNumber(j)) {
              bindResult =
                SCCoreGlue.scc_bind_double(mydbc, 1 + j,
                  batchData.getEntryBindColumnDouble(j));
            } else if (batchData.isEntryBindColumnString(j)) {
              final String text = batchData.getEntryBindColumnString(j);
              bindResult =
                SCCoreGlue.scc_bind_text_utf16_bytes(mydbc, 1 + j,
                  text, text.length() * 2);
            } else {
              bindResult =
                SCCoreGlue.scc_bind_null(mydbc, 1 + j);
            }
          }

          if (bindResult != 0) {
            batchResults.startNewEntry();
            batchResults.entryPutFieldAsInteger("status", 1); // SQLite ERROR 1
            batchResults.entryPutFieldAsString("message",
            SCCoreGlue.scc_get_last_error_message(mydbc));
            batchResults.putNewEntry();
            SCCoreGlue.scc_end_statement(mydbc);
            continue;
          }

          int stepResult = SCCoreGlue.scc_step(mydbc);

          if (stepResult == 100) {
            final int columnCount = SCCoreGlue.scc_get_column_count(mydbc);

            batchResults.startNewEntryWithRows();

            for (int j=0; j < columnCount; ++j) {
              batchResults.entryPutColumnName(SCCoreGlue.scc_get_column_name(mydbc, j));
            }

            do {
              batchResults.startEntryRow();

              for (int col=0; col < columnCount; ++col) {
                final int type = SCCoreGlue.scc_get_column_type(mydbc, col);

                if (type == SCCoreGlue.SCC_COLUMN_TYPE_INTEGER ||
                    type == SCCoreGlue.SCC_COLUMN_TYPE_FLOAT) {
                  batchResults.entryPutRowValueAsDouble(
                    SCCoreGlue.scc_get_column_double(mydbc, col));
                } else if (type == SCCoreGlue.SCC_COLUMN_TYPE_NULL) {
                  batchResults.entryPutRowValueAsNull();
                } else {
                  batchResults.entryPutRowValueAsString(
                    SCCoreGlue.scc_get_column_text(mydbc, col));
                }
              }

              batchResults.entryPutRow();

              stepResult = SCCoreGlue.scc_step(mydbc);
            } while (stepResult == 100);

            batchResults.putNewEntryWithRows();
            SCCoreGlue.scc_end_statement(mydbc);
          } else if (stepResult == 101) {
            int totalChanges = SCCoreGlue.scc_get_total_changes(mydbc);
            int rowsAffected = totalChanges - previousTotalChanges;

            batchResults.startNewEntry();
            // same order as iOS & macOS ("osx"):
            batchResults.entryPutFieldAsInteger("status", 0); // SQLite OK
            batchResults.entryPutFieldAsInteger("totalChanges", totalChanges);
            batchResults.entryPutFieldAsInteger("rowsAffected", rowsAffected);
            batchResults.entryPutFieldAsInteger("lastInsertRowId",
              SCCoreGlue.scc_get_last_insert_rowid(mydbc));
            batchResults.putNewEntry();
            SCCoreGlue.scc_end_statement(mydbc);
          } else {
            batchResults.startNewEntry();
            batchResults.entryPutFieldAsInteger("status", 1); // SQLite ERROR 1
            batchResults.entryPutFieldAsString("message",
              SCCoreGlue.scc_get_last_error_message(mydbc));
            batchResults.putNewEntry();
            SCCoreGlue.scc_end_statement(mydbc);
          }
        }
      }
    } catch(Exception e) {
      // NOT EXPECTED - internal error:
      throw new RuntimeException(e);
    }
  }

  static {
    System.loadLibrary("sqlc-connection-core-glue");
    SCCoreGlue.scc_init();
  }
}
