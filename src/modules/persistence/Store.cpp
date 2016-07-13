/**
 * @file
 */

#include "Store.h"
#include "core/Log.h"
#include <sstream>

namespace persistence {

Store::State::State(PGresult* _res) :
		res(_res) {
}

Store::State::~State() {
	if (res != nullptr) {
		PQclear(res);
	}
}

Store::Store(Connection* conn) :
		_connection(conn) {
}

Store::~Store() {
	_connection = nullptr;
}

bool Store::store(const PeristenceModel& model) const {
	const std::string& insertSql = sqlBuilder(model, false);
	return query(insertSql).result;
}

bool Store::createTable(const PeristenceModel& model) const {
	const std::string& crSql = model.getCreate();
	Log::info("create table '%s'", model.getTableName().c_str());
	return query(crSql).result;
}

KeyValueMap Store::load(const PeristenceModel& model) const {
	const std::string& loadSql = sqlLoadBuilder(model);
	Log::trace("sql used %s", loadSql.c_str());
	KeyValueMap dbResult;
	const State& state = query(loadSql);
	if (state.result && state.affectedRows == 1) {
		const int nFields = PQnfields(state.res);
		for (int i = 0; i < nFields; ++i) {
			const char* name = PQfname(state.res, i);
			const char* value = PQgetvalue(state.res, 0, i);
			dbResult[std::string(name)] = std::string(value);
		}
	}

	return dbResult;
}

std::string Store::sqlBuilder(const PeristenceModel& model, bool update) const {
	std::stringstream insertSql;
	insertSql << "INSERT INTO " << model.getTableName() << " ";
	std::stringstream fieldKeys;
	std::stringstream valueKeys;

	const Fields& fields = model.getFields();

	std::string add = "";
	for (auto p = fields.begin(); p != fields.end(); ++p) {
		const std::string& strKey = p->first;
		const std::string& strValue = p->second;
		if (!model.isSerial(strKey)) {
			fieldKeys << add << strKey;
			valueKeys << add << "'" << strValue << "'";
			add = ", ";
		}
	}

	insertSql << "(" << fieldKeys.str() << ") VALUES (" << valueKeys.str() << ");";
	const std::string& str = insertSql.str();
	Log::trace("used query %s", str.c_str());
	return str;
}

std::string Store::sqlLoadBuilder(const PeristenceModel& model) const {
	std::string loadSql = "SELECT * FROM " + model.getTableName() + " ";
	std::string fieldKeys;

	const Fields& fields = model.getFields();

	std::string add;
	for (auto p = fields.begin(); p != fields.end(); ++p) {
		const std::string& strKey = p->first;
		const std::string& strValue = p->second;
		if (!model.isSerial(strKey)) {
			fieldKeys += add + strKey + " = '" + strValue + "'";
			if (add.empty()) {
				add = " AND ";
			}
		}
	}

	loadSql += "WHERE " + fieldKeys + ";";
	return loadSql;
}

bool Store::begin() const {
	return query("BEGIN").result;
}

bool Store::end() const {
	return query("END").result;
}

bool Store::checkLastResult(State& state) const {
	state.affectedRows = 0;
	if (state.res == nullptr) {
		return false;
	}

	state.lastState = PQresultStatus(state.res);

	switch (state.lastState) {
	case PGRES_EMPTY_QUERY:
	case PGRES_BAD_RESPONSE:
	case PGRES_FATAL_ERROR:
		state.lastErrorMsg = PQerrorMessage(_connection->connection());
		Log::error("Failed to execute sql: %s ", state.lastErrorMsg.c_str());
		return false;
	case PGRES_COMMAND_OK:
		break;
	case PGRES_TUPLES_OK:
		state.affectedRows = PQntuples(state.res);
		Log::trace("Affected rows on read %i", state.affectedRows);
		break;
	default:
		Log::error("not catched state: %s", PQresStatus(state.lastState));
		return false;
	}

	state.result = true;
	return true;
}

Store::State Store::query(const std::string& query) const {
	Log::trace("Query: %s", query.c_str());
	State s(PQexec(_connection->connection(), query.c_str()));
	checkLastResult(s);
	return s;
}

}
