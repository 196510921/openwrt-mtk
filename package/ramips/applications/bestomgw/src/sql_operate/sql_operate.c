#include "sql_operate.h"
#include "sql_table.h"
#include "m1_common_log.h"
/*prototypy*******************************************************************************************************/

/*variable*******************************************************************************************************/


void sql_rollback(sqlite3* db)
{
	int rc;
	char* errorMsg = NULL;

	if(sqlite3_exec(db, "ROLLBACK", NULL, NULL, &errorMsg) == SQLITE_OK){
	    M1_LOG_INFO("ROLLBACK ok！\n");
	}else{
	    M1_LOG_ERROR("ROLLBACK failed！\n");
	}

	if(errorMsg)
		sqlite3_free(errorMsg);
}

int sql_step(sqlite3* db, char* sql)
{
	int rc;
	sqlite3_stmt* stmt = NULL;

	if(SQLITE_OK != sqlite3_prepare_v2(db ,sql ,strlen(sql) ,&stmt ,NULL)){
		if(stmt)
			sqlite3_finalize(stmt);
		return SQL_OPERATE_FAILED;
	}
	
	if(rc = sqlite3_step(stmt) != SQLITE_DONE){
		M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
	    if(rc == SQLITE_BUSY || rc == SQLITE_MISUSE || rc == SQLITE_LOCKED || rc == SQLITE_CORRUPT)
	        rollback(db);

		sqlite3_finalize(stmt);
		return SQL_OPERATE_FAILED;
	}

	if(stmt)
		sqlite3_finalize(stmt);

	M1_LOG_DEBUG("SQLITE3: step ok！\n");
	return SQL_OPERATE_SUCCESS;
}

/*事物开始*/
int sql_begin_transaction(sqlite3* db)
{
	const char * sql = "BEGIN" ;

	if(sql_step(db, sql) != SQL_OPERATE_SUCCESS){
		M1_LOG_DEBUG("SQLITE3: BEGIN failed！\n");
		sql_rollback(db);
		return SQL_OPERATE_FAILED;
	}

	M1_LOG_DEBUG("SQLITE3: BEGIN ok！\n");
	return SQL_OPERATE_SUCCESS; 
}

/*提交事物*/
int sql_commit_transaction(sqlite3* db)
{
	const char * sql = "COMMIT" ;

	if(sql_step(db, sql) != SQL_OPERATE_SUCCESS){
		M1_LOG_DEBUG("SQLITE3: COMMIT failed！\n");
		return SQL_OPERATE_FAILED;
	}
	
	M1_LOG_DEBUG("SQLITE3: COMMIT ok！\n");
	return SQL_OPERATE_SUCCESS; 
}



#if 0
/*sqlite3 insert*/
static int insert(sql_operate data)
{
	sqlite3 * conn = NULL ;
	int nRet ;
	const char * beginSQL = "BEGIN TRANSACTION" ;
	const char * commitSQL = "COMMIT" ;

	char insertSqlCommand[100] ;
	
	// 打开数据库文件
	nRet = sqlite3_open( szFilePath , &conn ) ;
	if( nRet != SQLITE_OK )
	{
		printf("加载数据库文件失败!\n") ;
		sqlite3_close( conn ) ;
		return 0 ;
	}
	
	// 显式开启一个事物
	sqlite3_stmt * stmt = NULL ;
	if( SQLITE_OK != sqlite3_prepare_v2( conn , beginSQL , strlen( beginSQL ) , &stmt , NULL ) )
	{
		if( stmt )
		{
			sqlite3_finalize( stmt ) ;
		}
		sqlite3_close( conn ) ;
		return 0 ;
	}
	
	if( sqlite3_step( stmt ) != SQLITE_DONE ) 
	{
		sqlite3_finalize( stmt ) ;
		sqlite3_close( conn ) ;
		return 0 ;
	}
	
	// 构建基于绑定变量的插入数据
	if( INSERT_BOOK == nOpMode )
	{
		strcpy( insertSqlCommand , "insert into Book values ( ? , ? , ? , ? , ? )" ) ;
	}
	else if( INSERT_CUSTOMER == nOpMode )
	{
		strcpy( insertSqlCommand , "insert into Customer values( ? , ? , ? , ? , ?)" ) ;
	}
	else if( INSERT_ORDER == nOpMode )
	{
		strcpy( insertSqlCommand , "insert into OrderInfo values( ? , ? , ? , ? )" ) ;
	}
	else
	{
		return 0 ;
	}

	sqlite3_stmt * stmt2 = NULL ;
	if( SQLITE_OK != sqlite3_prepare_v2( conn , insertSqlCommand , strlen( insertSqlCommand ) , &stmt2 , NULL ) )
	{
		if( stmt2 )
		{
			sqlite3_finalize(stmt2 );
			sqlite3_close( conn ) ;
			return 0 ;
		}
	}

	if( INSERT_BOOK == nOpMode )
	{
		sqlite3_bind_text( stmt2 , 1 , ((Book*)pData)->id , strlen( ((Book*)pData)->id) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_text( stmt2 , 2 , ((Book*)pData)->name , strlen( ((Book*)pData)->name ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_text( stmt2 , 3 , ((Book*)pData)->type , strlen( ((Book*)pData)->type ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_double( stmt2 , 4 , ((Book*)pData)->price ) ;
		sqlite3_bind_text( stmt2 , 5 , ((Book*)pData)->content , strlen( (( Book*)pData)->content ) , SQLITE_TRANSIENT ) ;
	}
	else if( INSERT_CUSTOMER == nOpMode )
	{
		sqlite3_bind_text( stmt2 , 1 , (( Customer*)pData)->id , strlen( (( Customer*)pData)->id ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_text( stmt2 , 2 , (( Customer*)pData)->name , strlen( (( Customer*)pData)->name ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_text( stmt2 , 3 , (( Customer*)pData)->sex , strlen( (( Customer*)pData)->sex ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_int( stmt2 , 4 ,  (( Customer*)pData)->age ) ;
		sqlite3_bind_text( stmt2 , 5 , (( Customer*)pData)->addr , strlen( (( Customer*)pData)->addr ) , SQLITE_TRANSIENT )  ;

	}
	else if( INSERT_ORDER == nOpMode )
	{
		sqlite3_bind_text( stmt2 , 1 , ( ( OrderInfo*)pData)->orderId , strlen( ( (OrderInfo*)pData)->orderId ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_text( stmt2 , 2 , ( ( OrderInfo*)pData)->buyerId , strlen( ( (OrderInfo*)pData)->buyerId ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_text( stmt2 , 3 , ( ( OrderInfo*)pData)->bookId ,  strlen( ( (OrderInfo*)pData)->bookId ) , SQLITE_TRANSIENT ) ;
		sqlite3_bind_text( stmt2 , 4 , ( ( OrderInfo*)pData)->buyType , strlen( ( (OrderInfo*)pData)->buyType ) , SQLITE_TRANSIENT ) ;
	}
	else
	{
		return 0 ;
	}
	
	if( SQLITE_DONE != sqlite3_step( stmt2 ) )
	{
		sqlite3_finalize( stmt2 ) ;
		sqlite3_close( conn ) ;
		return 0 ;
	}
	
	// 重新初始化该sqlite3_stmt绑定的对象
	sqlite3_reset( stmt2 ) ;
	sqlite3_finalize( stmt2 ) ;
	
	// 提交之前的事物
	sqlite3_stmt * stmt3 = NULL ;
	if( SQLITE_OK != sqlite3_prepare_v2( conn , commitSQL , strlen( commitSQL ) , &stmt3  , NULL ) ) 
	{
		sqlite3_finalize( stmt3 ) ;
		sqlite3_close( conn ) ;
		return 0 ;
	}
	
	if( sqlite3_step( stmt3 ) != SQLITE_DONE )
	{
		if( stmt3 ) 
		{
			sqlite3_finalize( stmt3 ) ;
		}
		sqlite3_close( conn ) ;
		return 0 ;
	}

	// 关闭数据库
	sqlite3_close( conn ) ;  
	
	return 1 ;
}

/*sqlite3 delete*/
static int delete(sql_operate data)
{
	sqlite3 * conn = NULL ;
	sqlite3_stmt * stmt = NULL ;
	int nRet ;
	char szSqlCommand[100] ;

	nRet = sqlite3_open( szFileName , &conn ) ;
	if( nRet != SQLITE_OK )
	{
		sqlite3_close( conn ) ;
		return 0 ;
	}

	if( nOpMode == DELETE_BOOK_BY_BOOKID )
	{
		sprintf( szSqlCommand , "delete from Book where id = %s ;" , szKeyword ) ;
	}
	else if( nOpMode == DELETE_BOOK_BY_BOOKNAME )
	{
		sprintf( szSqlCommand , "delete from Book where name = %s ;" , szKeyword ) ;
	}
	else if( nOpMode == DELETE_BOOK_ALL_ITEMS )
	{
		sprintf( szSqlCommand , "delete from Book ; " ) ;
	}
	else if( nOpMode == DELETE_CUSTOMER_BY_ID )
	{
		sprintf( szSqlCommand , "delete from Customer where id = %s ;" , szKeyword ) ;
	}
	else if( nOpMode == DELETE_CUSTOMER_BY_NAME )
	{
		sprintf( szSqlCommand , "delete from Customer where name = %s ;" , szKeyword ) ;
	}
	else if( nOpMode == DELETE_CUSTOMER_ALL_ITEMS )
	{
		sprintf( szSqlCommand , "delete from Customer ;") ;
	}
	else if( nOpMode == DELETE_ORDER_BY_ORDERID )
	{
		sprintf( szSqlCommand , "delete from OrderInfo where orderid = %s ;" , szKeyword ) ;
	}
	else if( nOpMode == DELETE_ORDER_BY_BOOKID )
	{
		sprintf( szSqlCommand , "delete from OrderInfo where bookid = %s ;" , szKeyword ) ;
	}
	else if( nOpMode == DELETE_ORDER_BY_CUSTOMERID )
	{
		sprintf( szSqlCommand , "delete from OrderInfo where customerid = %s ;" , szKeyword ) ;
	}
	else if( nOpMode == DELETE_ORDER_ALL_ITEMS )
	{
		sprintf( szSqlCommand , "delete from orderInfo ;" ) ;
	}
	else
	{
		return 0 ;
	}

	if( SQLITE_OK != sqlite3_prepare_v2( conn , szSqlCommand , strlen( szSqlCommand ) , & stmt , NULL ) ) 
	{
		if( stmt )
		{
			sqlite3_finalize( stmt ) ;
		}
		sqlite3_close( conn ) ;
		return 0 ;
	}

	if( sqlite3_step( stmt ) != SQLITE_DONE )
	{
		if( stmt ) 
		{
			sqlite3_finalize( stmt ) ;
		}
		sqlite3_close( conn ) ;
		return 0 ;
	}

	// 关闭数据库
	sqlite3_close( conn ) ;  

	return 1 ;
}

/*sqlite3 select*/
static int query(sql_operate data)
{
	sqlite3 * conn = NULL ;
	int nRet = 0 ;
	sqlite3_stmt * stmt1 = NULL ;

	char szQueryCommand[100] ;
	if( QUERY_BOOK_BY_BOOKID == nOpMode )
	{
		sprintf( szQueryCommand , "select * from Book where id = %s ;" , szKeyword ) ;
	}
	else if( QUERY_BOOK_BY_BOOKNAME == nOpMode )
	{
		sprintf( szQueryCommand , "select * from Book where name = %s ;" , szKeyword  ) ;
	}
	else if( QUERY_CUSTOMER_BY_ID == nOpMode )
	{
		sprintf( szQueryCommand , "select * from Customer where id = %s ;" , szKeyword ) ;
	}
	else if( QUERY_CUSTOMER_BY_NAME == nOpMode )
	{
		sprintf( szQueryCommand , "select * from Customer where name = %s ;" , szKeyword ) ;
	}
	else if( QUERY_ORDER_ORDERID == nOpMode )
	{
		sprintf( szQueryCommand , "select * from OrderInfo where orderid = %s ;" , szKeyword ) ;
	}
	else if( QUERY_ORDER_CUSTOMERID == nOpMode )
	{
		sprintf( szQueryCommand , "select * from OrderInfo where customerid = %s ;" , szKeyword ) ;
	}
	else if( QUERY_ORDER_BOOKID == nOpMode )
	{
		sprintf( szQueryCommand , "select * from OrderInfo where bookid = %s ;" , szKeyword ) ;
	}
	else if( QUERY_BOOK_ALL_INFO == nOpMode )
	{
		sprintf( szQueryCommand , "select * from Book ;") ;
	}
	else if( QUERY_CUSTOMER_ALL_INFO == nOpMode )
	{
		sprintf( szQueryCommand , "select * from Customer ;") ;
	}
	else if( QUERY_ORDER_ALL_INFO == nOpMode )
	{
		sprintf( szQueryCommand , "select * from OrderInfo ;") ;
	}
	else if( QUERY_USERS_BY_USERNAME == nOpMode )
	{
		sprintf( szQueryCommand , "select * from UserStatus where name = %s ;" , szKeyword ) ;
	}
	else
	{
		return 0 ;
	}

	nRet = sqlite3_open( szFilePath , &conn ) ;
	if( nRet != SQLITE_OK )
	{
		sqlite3_close( conn ) ;
		return 0 ;
	}

	char **dbResult ;
	int  nRow ;
	int  nColumn ;
	char *errorMsg = NULL ;
	int index ;

	nCount = 0 ;

	nRet = sqlite3_get_table( conn , szQueryCommand , &dbResult , &nRow , &nColumn , &errorMsg  ) ;
	if( nRet == SQLITE_OK )
	{
		index = nColumn ;

		if( nRow == 0 )
		{
			return 1 ;
		}

		if( QUERY_BOOK_ALL_INFO == nOpMode || QUERY_BOOK_BY_BOOKID == nOpMode|| QUERY_BOOK_BY_BOOKNAME == nOpMode )
		{
			for ( int j = 0 ; j < nRow ; j++ )
			{
				for( int i = 0 ; i < nColumn ; i++ )
				{
					if( 0 == ( i % 5 ) )
					{
						strcpy( ((Book*)pData)[nCount].id ,   dbResult[index++] ) ;
					}
					if( 1 == ( i % 5 )  )
					{
						strcpy( ((Book*)pData)[nCount].name , dbResult[index++] ) ;
					}
					if( 2 == ( i % 5 ) )
					{
						strcpy( ((Book*)pData)[nCount].type , dbResult[index++] ) ;
					}
					if( 3 == ( i % 5 )  )
					{
						double tmp = atof( dbResult[index++] ) ;
						((Book*)pData)[nCount].price = tmp ;
					}
					if( 4 == ( i % 5 ) )
					{
						strcpy( ((Book*)pData)[nCount].content , dbResult[index++] ) ;
						
						nCount++ ;
					}
				}
			}
		}
		else if( QUERY_CUSTOMER_ALL_INFO == nOpMode || QUERY_CUSTOMER_BY_ID == nOpMode || QUERY_CUSTOMER_BY_NAME == nOpMode )
		{
			for( int j = 0 ; j < nRow ; j++ )
			{
				for( int i = 0 ; i < nColumn ; i++ )
				{
					if( 0 == ( i % 5 ) )
					{
						strcpy( ( ( Customer*)pData)[nCount].id , dbResult[index++] ) ;
					}
					if( 1 == ( i % 5 ) )
					{
						strcpy( ( ( Customer*)pData)[nCount].name, dbResult[index++] ) ;
					}
					if( 2 == ( i % 5 ) )
					{
						strcpy( ( ( Customer*)pData)[nCount].sex , dbResult[index++] ) ;
					}
					if( 3 == ( i % 5 ) )
					{
						int tmp = atoi( dbResult[index++] ) ;
						(( Customer*)pData)[nCount].age = tmp ;
					}
					if( 4 == ( i % 5 ) )
					{
						strcpy( ( ( Customer*)pData)[nCount].addr , dbResult[index++] ) ;
						
						nCount++ ;
					}
				}
			}
		}
		else if( QUERY_ORDER_ALL_INFO == nOpMode || QUERY_ORDER_ORDERID == nOpMode || QUERY_ORDER_CUSTOMERID == nOpMode 
			|| QUERY_ORDER_BOOKID == nOpMode )
		{
			for( int j = 0 ; j < nRow ; j++ )
			{
				for( int i = 0 ; i < nColumn ; i++ )
				{
					if( 0 == ( i % 5 ) )
					{
						strcpy( (( OrderInfo * )pData)[nCount].orderId , dbResult[index++] ) ; 
					}
					
					if( 1 == ( i % 5 ) )
					{
						strcpy( (( OrderInfo *)pData )[nCount].buyerId , dbResult[index++] ) ; 
					}

					if( 2 == ( i % 5 ) )
					{
						strcpy( (( OrderInfo *)pData)[nCount].bookId , dbResult[index++] ) ;
					}
					if( 3 == ( i % 5 ) )
					{
						strcpy( (( OrderInfo *)pData)[nCount].buyType , dbResult[index++] ) ;

						nCount++ ;
					}
				}
			}
		}
		else if( QUERY_USERS_BY_USERNAME == nOpMode )
		{
			for( int j = 0 ; j < nRow ; j++ )
			{
				for( int i = 0 ; i < nColumn ; i++ )
				{
					if( 0 == ( i % 5 ) )
					{
						strcpy( (( UserStatus*)pData)[nCount].userId , dbResult[index++] ) ;
					}
					if( 1 == ( i % 5 ) )
					{
						strcpy( (( UserStatus*)pData)[nCount].userName , dbResult[index++] ) ;
					}
					if( 2 == ( i % 5 ) )
					{
						strcpy( (( UserStatus*)pData)[nCount].password , dbResult[index++] ) ;
					}
					if( 3 == ( i % 5 ) )
					{
						strcpy( (( UserStatus*)pData)[nCount].loginType , dbResult[index++] ) ;	

						nCount ++ ;
					}
				}
			}
		}
		else
		{
			return 0 ;
		}
	}

	// 释放数据
	sqlite3_free_table( dbResult ) ;

	// 关闭数据库
	sqlite3_close( conn ) ;  

	return 1 ;
}

/*sqlite3 update*/
static int modify(sql_operate data)
{
	sqlite3 * conn = NULL ;
	sqlite3_stmt * stmt = NULL ;
	int nRet = 0 ;

	char szSqlCommnad[200] ;

	nRet = sqlite3_open( szFileName , &conn ) ;
	if( SQLITE_OK != nRet )
	{
		sqlite3_close( conn ) ;
		return 0 ;
	}

	if( MODIFY_BOOK == nOpMode )
	{
		sprintf( szSqlCommnad , "update Book set id = %s , name = %s , type = %s , price = %lf , content = %s \
			where id = %s ;" , ( (Book*)pData)->id , ( (Book*)pData)->name , ( (Book*)pData)->type , ( (Book*)pData)->price , \
			( (Book*)pData)->content , szKeyword ) ;
	}
	else if( MODIFY_CUSTOMER == nOpMode )
	{
		sprintf( szSqlCommnad , "update Customer set id = %s , name = %s , sex = %s , age = %d , address= %s \
			where id = %s ; " , (( Customer *)pData )->id , (( Customer *)pData )->name , (( Customer *)pData )->sex , \
			(( Customer *)pData )->age , (( Customer *)pData )->addr , szKeyword ) ;
	}
	else if( MODIFY_ORDER == nOpMode )
	{
		sprintf( szSqlCommnad , "update OrderInfo set orderid = %s , customerid = %s , bookid = %s , buytype = %s \
			where orderid = %s ;" , (( OrderInfo*)pData)->orderId , (( OrderInfo *)pData )->buyerId , (( OrderInfo *)pData )->bookId , \
			(( OrderInfo *)pData )->buyType , szKeyword ) ;
	}
	else
	{
		return 0 ;
	}

	if( SQLITE_OK != sqlite3_prepare_v2( conn , szSqlCommnad , strlen( szSqlCommnad ) , &stmt , NULL ) )
	{
		if( stmt )
		{
			sqlite3_finalize( stmt ) ;
		}
		sqlite3_close( conn ) ;
		return 0 ;

	}

	if( SQLITE_DONE != sqlite3_step( stmt) )
	{
		if( stmt )
		{
			sqlite3_finalize( stmt ) ;
		}
		sqlite3_close( conn ) ;
		return 0 ;
	}

	// 关闭数据库
	sqlite3_close( conn ) ;  

	return 1 ;
}

#endif







