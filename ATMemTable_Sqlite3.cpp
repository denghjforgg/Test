#pragma region region_COMPLIE_SQL_STATEMENT
CCriticalSection g_lckATMemTable_Sqlite3Tmp;
CATMemTable_Sqlite3 *g_ptrATMemTable_Sqlite3Tmp = nullptr;

//=========================================================================
// ×Ô¶¯×¢²á SQL±àÒëÓï¾ä Àà
//=========================================================================
typedef void(*CompileStatementProc)(bool bDelete);
class CCompileSqlStatementCollection
{
private:
	std::vector<CompileStatementProc> m_vectPtrProc;
public:

	void AddProc(CompileStatementProc pProc)
	{
		m_vectPtrProc.push_back(pProc);
	}
	void ProcAll(CATMemTable_Sqlite3 *pATMemTable_Sqlite3, bool bDelete = false)
	{
		g_ptrATMemTable_Sqlite3Tmp = pATMemTable_Sqlite3;

		CompileStatementProc pProc = nullptr;
		for (int i = m_vectPtrProc.size() - 1; i >= 0; --i)
		{
			pProc = m_vectPtrProc.at(i);
			(*pProc)(bDelete);
		}
	}
};
CCompileSqlStatementCollection g_compileSSCMem;

class CCompileSqlStatement
{
public:
	CCompileSqlStatement(CompileStatementProc pProc)
	{
		g_compileSSCMem.AddProc(pProc);
	}
};

/*-----------------------------
  REGISTER_SQL_STATEMENT ºê
  ÓÃÀ´×¢²á SQL±àÒëÓï¾ä
-----------------------------*/
#define REGISTER_SQL_STATEMENT(PROC_NAME, SQL) \
void ComplieStmt##PROC_NAME##(bool bDelete)\
{\
	if (!bDelete)\
	{\
		g_ptrATMemTable_Sqlite3Tmp->m_ptrStmt_##PROC_NAME->finalize();\
		delete g_ptrATMemTable_Sqlite3Tmp->m_ptrStmt_##PROC_NAME##;\
		g_ptrATMemTable_Sqlite3Tmp->m_ptrStmt_##PROC_NAME = nullptr;\
	}\
	else\
	{\
		g_ptrATMemTable_Sqlite3Tmp->m_ptrStmt_##PROC_NAME = new CppSQLite3Statement;\
		*g_ptrATMemTable_Sqlite3Tmp->m_ptrStmt_##PROC_NAME =\
			g_ptrATMemTable_Sqlite3Tmp->m_ptrMemSqlite3->compileStatement(SQL);\
	}\
}\
CCompileSqlStatement complieSqlMem##PROC_NAME##(ComplieStmt##PROC_NAME##);

#ifdef WIN64
#	define bindPtr bind
#else
#	define bindPtr bind64
#endif
#pragma endregion region_COMPLIE_SQL_STATEMENT

//=========================================================================
// class CATMemTable_Sqlite3
//=========================================================================
void CATMemTable_Sqlite3::PreConstruct()
{
	g_lckATMemTable_Sqlite3Tmp.Lock();
}
CATMemTable_Sqlite3::CATMemTable_Sqlite3()
{
	m_ptrMemSqlite3 = new CppSQLite3DB;
	InitTable();
	g_compileSSCMem.ProcAll(this);
}
void CATMemTable_Sqlite3::EndConstruct()
{
	g_lckATMemTable_Sqlite3Tmp.Unlock();
}
CATMemTable_Sqlite3::~CATMemTable_Sqlite3()
{
	g_compileSSCMem.ProcAll(this, true);

	delete m_ptrMemSqlite3;
	m_ptrMemSqlite3 = nullptr;
}

//++++++++++++++++++++++++++++++++Ë½ÓÐ+++++++++++++++++++++++++++++++++++++
void CATMemTable_Sqlite3::InitTable()
{
	m_ptrMemSqlite3->open(":memory:");

	m_ptrMemSqlite3->execDML("CREATE TABLE block (blkSrl INTEGER, blkIdx INTEGER, blkObj INTEGER);");
}

//++++++++++++++++++++++++++++++++¼Ì³Ð+++++++++++++++++++++++++++++++++++++
REGISTER_SQL_STATEMENT(Add_Block, "INSERT INTO block(blkSrl, blkIdx, blkObj) VALUES(?, ?, ?);")
void CATMemTable_Sqlite3::Add_Block(int iBlkIdx, CATBlock *pBlock)
{
	m_ptrStmt_Add_Block->bind(1, pBlock->GetATNodeNo());
	m_ptrStmt_Add_Block->bind(2, iBlkIdx);
	m_ptrStmt_Add_Block->bindPtr(3, PTR_TO_INT3264(pBlock));
	m_ptrStmt_Add_Block->execDML();
	m_ptrStmt_Add_Block->finalize();
}

