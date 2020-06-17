#import <Foundation/Foundation.h>

@interface SQLiteBatchCore : NSObject

+ (void) initialize;

+ (int) openBatchConnection: (NSString *) filename
                      flags: (int) flags;

+ (NSArray *) executeBatch: (int) connection_id
                      data: (NSArray *) data;

@end
