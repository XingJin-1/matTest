#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <experimental\filesystem>
#include <windows.h>
#include <map>
#include <tuple>
#include <codecvt>
#include "DataReader.h"
#include <clocale>
#include <time.h>
#include <chrono>
#include <mat.h>
#include <comdef.h>

namespace filesys = std::experimental::filesystem;
using namespace std;

wstring convert_to_lower(wstring data) {
	transform(data.begin(), data.end(), data.begin(),
		[](unsigned char c) { return tolower(c); });
	return data;
}

vector<wstring>get_corresponding_files(vector<wstring> file_match_conditions, vector<wstring> files) {
	DataReader dr;
	vector<wstring> matching_files{};
	for (auto file : files) {
		// check how many conditions are given in the current filename based on the number of '=' chars
		// there is always should be at least 1 occurence for dut_id / sample, the rest are for conditions
		int num_of_conds_in_filename = dr.count_char_occurence(file, L'=');
		// should be decreased by 2 since two extra = , one for dut_id, one for REP=
		num_of_conds_in_filename = num_of_conds_in_filename - 2;
		// add one more condition for matching parent folder name
		num_of_conds_in_filename++;
		// add one more condition for matching Report-Picture or Report-waveform names
		num_of_conds_in_filename++;
		int num_of_conds_matched = 0;
		for (auto file_match_cond : file_match_conditions) {
			// make change in order to align with ending 0s              111111111111111
			if (file_match_cond.find(L"=") != wstring::npos) {
				int pos_last_non_zero_digit = file_match_cond.find_last_of(L"123456789");
				int pos_decimal_symbol = file_match_cond.find_last_of(L".");
				if (pos_last_non_zero_digit > pos_decimal_symbol) {
					file_match_cond.erase(pos_last_non_zero_digit + 1, file_match_cond.length() - 2);
				}
				else {
					if (pos_decimal_symbol != -1)
						file_match_cond.erase(pos_decimal_symbol, file_match_cond.length() - 1);
				}
			}

			// count number of conditions that match with conditions in the filename
			if (convert_to_lower(file).find(convert_to_lower(file_match_cond)) != wstring::npos) {
				num_of_conds_matched++;
			}
		}
		// png file is matched if the number of total matched conditions are same as the number of 
		// conditions in the filename
		if (num_of_conds_matched == num_of_conds_in_filename) {
			wstring base_filename = wstring(file).substr(wstring(file).find_last_of(L"/\\") + 1);
			matching_files.push_back(base_filename);
		}
	}
	return matching_files;
}

vector<wstring> getAllFilesInDir(const wstring &dirPath, const wstring &fileExt)
{
	// Create a vector of wstring
	vector<wstring> listOfFiles;

	try {
		// Check if given path exists and points to a directory
		if (filesys::exists(dirPath) && filesys::is_directory(dirPath))
		{
			// Create a Recursive Directory Iterator object and points to the starting of directory
			filesys::recursive_directory_iterator iter(dirPath);

			// Create a Recursive Directory Iterator object pointing to end.
			filesys::recursive_directory_iterator end;

			// Iterate till end
			while (iter != end) {
				// Check if current entry is a directory and if exists in skip list
				if (filesys::is_directory(iter->path())) {
					// Skip the iteration of current directory pointed by iterator
				}
				else {
					// Add the name in vector
					try {
						if (iter->path().wstring().find(fileExt) != wstring::npos) {
							listOfFiles.push_back(iter->path().wstring());
						}
					}
					catch (invalid_argument &e) {
						error_code ec;
						iter.increment(ec);
					}
				}

				error_code ec;
				// Increment the iterator to point to next entry in recursive iteration
				iter.increment(ec);
				if (ec) {
					cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message() << '\n';
				}
			}

		}
	}
	catch (system_error & e) {
		cerr << "Exception :: " << e.what();
	}
	return listOfFiles;
}

wstring strrep(wstring line, char from, char to) {
	for (auto i = 0; i < line.size(); i++) {
		if (char(line[i]) == char(from)) {
			line[i] = to;
		}
	}
	return line;
}

// replace all special characters with _ to avoid Tembo crash
wstring validate_param_name(wstring raw_param_name) {
	wstring param_name = raw_param_name;
	param_name = strrep(param_name, '-', '_');
	param_name = strrep(param_name, '(', '_');
	param_name = strrep(param_name, ')', '_');
	param_name = strrep(param_name, '!', '_');
	param_name = strrep(param_name, '#', '_');
	param_name = strrep(param_name, ',', '_');
	param_name = strrep(param_name, '.', '_');
	// if first char is _, remove it
	if (param_name[0] == '_') {
		param_name.erase(0, 1);
	}
	return param_name;
}

wstring construct_recipe(wstring report_template, wstring report_name, wstring project_name) {
	return wstring(L"<Recipe><Name>Report Starter</Name>") +
		L"<ApplyTemplate href=&quot;rcodes/apply_report_template_to_data." +
		L"xsl&quot; enabled=&quot;true&quot;>" +
		L"<param><name>generatePDF</name><value>true</value></param>" +
		L"<param><name>reportTemplate</name><value>" + report_template + L"</value></param>" +
		L"<param><name>reportName</name><value>" + report_name + L"</value></param>" +
		L"<param><name>reportID</name><value /></param>" +
		L"<param><name>d</name><value /></param>" +
		L"<param><name>mapping</name><value /></param>" +
		L"<param><name>index</name><value>" + project_name + L"</value></param>" +
		L"<param><name>field</name><value>payload.*</value></param>" +
		L"<param><name>filter</name><value><filters>" +
		L"<filter><field>metaData.dataset_id</field><name>Dataset Id</name><filterType>MultiSelect</filterType><filterOperator /><filterUnit />" +
		L"<filterValues><filterValue>$dataset_id$</filterValue></filterValues></filter>" +
		L"<filter><field>metaData.data_object_type</field><name>Data Object Type</name><filterType>MultiSelect</filterType><filterOperator /><filterUnit />" +
		L"<filterValues><filterValue>value</filterValue></filterValues></filter></filters></value></param>" +
		L"<param><name>grouping</name><value /></param>" +
		L"<param><name>query</name><value>{      &quot;query&quot" +
		L";:{      &quot;bool&quot;: {      &quot;must&quot; :" +
		L"[            ]      }      }      }</value></param></ApplyTemplate></Recipe>";
}

int check_data_type(mxArray *cellArrayData) {
	// check the type of current cell. Possible: [], NaN, string, double
	// need different function to read different type of data, so we have to know what the type it is 
	// 1: []; 2:string; 3:NaN; number:double
	int return_value = 0;
	try {
		double *double_pointer = (double*)mxGetDoubles(cellArrayData);
		if (double_pointer == NULL)
			throw("current cell is string.");
		if (isnan(*double_pointer)) {
			return_value = 3;
		}
		else {
			return_value = 4;
		}
	}
	catch (...) {
		string *string_pointer = (string*)mxGetPr(cellArrayData);
		if (string_pointer == NULL) {
			return_value = 1;
		}
		else {
			return_value = 2;
		}
	}
	return return_value;
}

wstring mat_read_string(const mxArray *cellArrayData) {
	char pr[500];
	mwSize len;
	string string_middle;
	wstring out_wstring;
	mwSize num = mxGetNumberOfElements(cellArrayData);
	int res = mxGetString(cellArrayData, pr, num + 1);
	string_middle = pr;
	out_wstring = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(string_middle);
	return out_wstring;
}

wstring mat_read_double(mxArray *cellArrayData) {
	double out_double;
	double *double_pointer = (double*)mxGetDoubles(cellArrayData);
	out_double = *double_pointer;
	return to_wstring(out_double);
}

// row_array_data___deviation: add #row elements to get the next element along current row
// col_array_data___num_iteration: iterate through #col times
vector<wstring> get_whole_row_test_data(mxArray *pMxArrayData, vector<wstring> single_row, int current_row, int deviation, int num_iteration) {
	// have to invoke char_to_wstring
	for (auto i = 0; i < num_iteration; i++) {
		wstring cell_element{};
		mxArray *cellArrayData = NULL;
		int index = current_row + deviation * i;
		cellArrayData = mxGetCell(pMxArrayData, index);
		int type_indicator_int = check_data_type(cellArrayData);
		// type of current cell is []
		if (type_indicator_int == 1) {
			cell_element = L"";
		}
		// type of current cell is NaN
		else if (type_indicator_int == 3) {
			cell_element = L"NaN";
		}
		// type of current cell is string
		else if (type_indicator_int == 2) {
			cell_element = mat_read_string(cellArrayData);
		}
		// type of current cell is double
		else {
			cell_element = mat_read_double(cellArrayData);
		}
		single_row.push_back(cell_element);
	}
	return single_row;
}

//bool CSVReader::csvs_to_json(vector<wstring> csv_files, map<wstring, map<wstring, wstring>> limits_struct, \
							map<wstring, wstring> configs_struct, \
							wstring out_folder_path, vector<wstring> png_files, vector<wstring> mat_files)
map <wstring, wstring> construct_overall_meta_data(mxArray *pMxArrayMeta) {
	printf("Start: Processing overall metadata ..................................................\n");

	mxArray *pMxArrayMiddle = NULL;
	mxArray *pMxArrayAssign = NULL;
	wstring ws_assign;

	map <wstring, wstring> overall_meta_data;
	wstring username = L"";
	wstring product_sales_code = L"";
	wstring basic_type = L"";
	wstring product_design_step = L"";
	wstring package = L"";
	wstring dut_id = L"";
	wstring req_id = L"";
	wstring description = L"";
	wstring typical = L"";
	//wstring test_number = L"";
	// in meta.meas.Jama: api_id global_id
	wstring api_id = L"";
	wstring global_id = L"";
	// need to be clerified where to find this one 
	wstring test_program_name = L"";
	wstring testunit_version = L"";
	wstring email = L"";

	wstring generator = L"";
	wstring generator_version = L"";
	wstring generator_domain = L"";
	wstring simulator_name = L"";
	wstring simulation_type = L"";
	wstring netlist_label = L"";
	
	// construct ts_data_created
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	int day = aTime->tm_mday;
	int month = aTime->tm_mon + 1;
	int year = aTime->tm_year + 1900;
	bool month_one_digit = false;
	bool day__one_digit = false;
	if (month < 10)
		month_one_digit = true;
	if (day < 10)
		day__one_digit = true;
	wstring ts_data_created = to_wstring(year) + ((month_one_digit) ? to_wstring(0) : +L"") + to_wstring(month) + ((day__one_digit) ? to_wstring(0) : +L"") + to_wstring(day);
	overall_meta_data[L"ts_data_created"] = ts_data_created;
	// -----------------------------------------------start: get metadata from meta.dut-----------------------------------------------
	
	pMxArrayMiddle = mxGetField(pMxArrayMeta, 0, "dut");

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "product_sales_code");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"product_sales_code"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "basic_type");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"basic_type"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "product_design_step");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"product_design_step"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "package");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"package"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "dut_id");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"dut_id"] = ws_assign;
	
	// -----------------------------------------------end: get metadata from meta.dut-----------------------------------------------

	// -----------------------------------------------start: get metadata from meta.meas----------------------------------------------
	// need to evaluate how the dimension of the array. like here is 2, so name1 + name2
	
	pMxArrayMiddle = mxGetField(pMxArrayMeta, 0, "meas");

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "user");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"user"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "user_name");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"username"] = ws_assign;

	//overall_meta_data[L"user"] = overall_meta_data[L"username"];

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "user_email_address");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"email"] = ws_assign;

	/*
	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "Jama");
	pMxArrayAssign = mxGetField(pMxArrayAssign, 0, "api_id");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"api_id"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "Jama");
	pMxArrayAssign = mxGetField(pMxArrayAssign, 0, "global_id");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"global_id"] = ws_assign;
	*/
	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "Jama");
	pMxArrayAssign = mxGetField(pMxArrayAssign, 0, "a");
	pMxArrayAssign = mxGetField(pMxArrayAssign, 0, "api_id");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"api_id"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "Jama");
	pMxArrayAssign = mxGetField(pMxArrayAssign, 0, "a");
	pMxArrayAssign = mxGetField(pMxArrayAssign, 0, "global_id");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"global_id"] = ws_assign;


	
	// -----------------------------------------------end: get metadata from meta.meas---------------------------------------------

	// -----------------------------------------------start: get metadata from meta.sw-----------------------------------------------
	// need to evaluate how the dimension of the array. like here is 2, so name1 + name2
	// mxGetN(pMxArrayMiddle) is 2
	pMxArrayMiddle = mxGetField(pMxArrayMeta, 0, "sw");

	/*
	// check the dimension of meta.sw dynamically 
	int dimension_sw = mxGetN(pMxArrayMiddle);
	for (int i = 0; i < dimension_sw; i++) {
		pMxArrayAssign = mxGetField(pMxArrayMiddle, i, "name");
		ws_assign = mat_read_string(pMxArrayAssign);
		overall_meta_data[L"sw_name" + to_wstring(i)] = ws_assign;
	}
	*/

	const char *fieldname;

	int nfields_sw = mxGetNumberOfFields(pMxArrayMiddle);
	for (int i = 0; i < nfields_sw; i++) {
		fieldname = mxGetFieldNameByNumber(pMxArrayMiddle, i);
		cout << "-------------------------crt fieldname in sw:" << fieldname << endl;
		pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, fieldname);
		pMxArrayAssign = mxGetField(pMxArrayAssign, 0, "name");
		ws_assign = mat_read_string(pMxArrayAssign);
		overall_meta_data[L"sw_name" + to_wstring(i)] = ws_assign;
	}


	/*
	pMxArrayAssign = mxGetField(pMxArrayMiddle, 0, "name");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"sw_name1"] = ws_assign;

	pMxArrayAssign = mxGetField(pMxArrayMiddle, 1, "name");
	ws_assign = mat_read_string(pMxArrayAssign);
	overall_meta_data[L"sw_name2"] = ws_assign;
	*/
	// -----------------------------------------------end: get metadata from meta.sw-----------------------------------------------
	overall_meta_data[L"testunit_version"] = L"1";

	cout << "-------------------------length of overall_meta_data:" << overall_meta_data.size() << endl;
	for (map <wstring, wstring> ::value_type& iter : overall_meta_data) {
		wcout << iter.first << ":" << iter.second << endl;
	}

	printf("End: Processing overall metadata ..................................................\n");
	return overall_meta_data;
}
// used for test
map <wstring, wstring> hardcode_overall_metadata() {
	map <wstring, wstring> overall_meta_data;
	overall_meta_data[L"username"] = L"Xing Jin";
	overall_meta_data[L"basic_type"] = L"S1234";
	overall_meta_data[L"product_sales_code"] = L"TLS1234";
	overall_meta_data[L"product_design_step"] = L"A21";
	overall_meta_data[L"package"] = L"TSON10";
	overall_meta_data[L"dut_id"] = L"1";
	//test_program_name get from name of right click 
	overall_meta_data[L"test_program_name"] = L".matTest";
	overall_meta_data[L"testunit_version"] = L"1";
	overall_meta_data[L"api_id"] = L"923";
	overall_meta_data[L"global_id"] = L"GID942";
	overall_meta_data[L"email"] = L"Jin.Xing@infineon.com";

	// construct ts_data_created
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	int day = aTime->tm_mday;
	int month = aTime->tm_mon + 1;
	int year = aTime->tm_year + 1900;
	bool month_one_digit = false;
	bool day__one_digit = false;
	if (month < 10)
		month_one_digit = true;
	if (day < 10)
		day__one_digit = true;
	wstring ts_data_created = to_wstring(year) + ((month_one_digit) ? to_wstring(0) : +L"") + to_wstring(month) + ((day__one_digit) ? to_wstring(0) : +L"") + to_wstring(day);
	overall_meta_data[L"ts_data_created"] = ts_data_created;

	return overall_meta_data;
}

// will be called within test_data_reader
map <wstring, wstring> construct_common_meta_data(map <wstring, wstring> overall_meta_data) {
	map <wstring, wstring> common_meta_data;

	common_meta_data[L"basic_type"] = overall_meta_data[L"basic_type"];
	common_meta_data[L"product_design_step"] = overall_meta_data[L"product_design_step"];
	common_meta_data[L"product_sales_code"] = overall_meta_data[L"product_sales_code"];
	common_meta_data[L"ts_data_created"] = overall_meta_data[L"ts_data_created"];
	common_meta_data[L"generator"] = L"C++";
	common_meta_data[L"generator_version"] = L"V11";
	common_meta_data[L"generator_domain"] = L"CV";
	common_meta_data[L"simulator_name"] = L"simulator";
	common_meta_data[L"simulation_type"] = L"type";
	common_meta_data[L"data_object_type"] = L"value";
	common_meta_data[L"data_object_type_version"] = L"1";
	common_meta_data[L"netlist_label"] = L"netlist_label";
	common_meta_data[L"user_name"] = overall_meta_data[L"username"];
	common_meta_data[L"user_email_address"] = overall_meta_data[L"email"];

	return common_meta_data;
}

// pass also meta data
bool test_data_reader(mxArray *pMxArrayData, map <wstring, wstring> overall_meta_data, map <wstring, wstring> configs_struct, wstring out_folder_path, wstring path_mat_data, vector<wstring> png_files, vector<wstring> mat_wfm_files) {
	printf("Start: Processing Test Data ..................................................\n");
	
	DataReader dr;
	// define header struct
	map<wstring, wstring> header_struct;
	header_struct[L"version"] = L"1.0.1";

	// build common_meta_data based on overall_meta_data
	map <wstring, wstring> common_meta_data = construct_common_meta_data(overall_meta_data);

	// define data_objects as array of maps
	// map<wstring, map<wstring, map<wstring, wstring>>> data_objects;
	vector<map<wstring, map<wstring, wstring>>> data_objects;

	wcout << L"Reading .mat file: " << endl;
	int num_dataset = mxGetN(pMxArrayData);
	mxArray *pMxArrayDataSubset = NULL;
	mxArray *pMxArrayIdSubset = NULL;
	wstring ws_id;
	for (int i = 0; i < num_dataset; i++) {
		// iterate through all datasets 
		int test_data_rows = 0;
		map<wstring, map<wstring, wstring>> limits_struct;

		pMxArrayDataSubset = mxGetField(pMxArrayData, i, "data");
		pMxArrayIdSubset = mxGetField(pMxArrayData, i, "id");
		ws_id = mat_read_string(pMxArrayIdSubset);
		cout << "BBBBBBBBBBBBBBBBBBB!!!!!!!!!!!!dimension of the data !!!!!!!!!!BBBBBBBBBBBBBBBBBBBBBB" << endl;
		cout << "dimension of data structure:" << mxGetM(pMxArrayDataSubset) << "___" << mxGetN(pMxArrayDataSubset) << endl;

		int row_array_data = mxGetM(pMxArrayDataSubset);
		int col_array_data = mxGetN(pMxArrayDataSubset);
		mxArray *cellArrayData = NULL;

		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// define structure to keep repeated condition data for output
		map<wstring, map<wstring, vector<int>>> repeated_conds;
		bool cond_repetition = false;

		wstring req_id = L"";
		wstring description = L"";
		wstring typical = L"";
		wstring test_number = L"";

		vector <wstring> column_types;
		// column_types -> field
		vector <wstring> field;
		// new fields---begin
		vector <wstring> tag;
		vector <wstring> sbfields;
		//vector <wstring> description;
		// new fields---end
		vector <wstring> usl;
		vector <wstring> lsl;
		// units -> unit
		vector <wstring> unit_meta;
		// variables -> name
		vector <wstring> name;
		vector <wstring> test_data;
		vector <wstring> no_limit_match;
		wstring type_indicator_ws;
		int type_indicator_int;

		// define struct to store unique out paramters(e.g.uniq('ibat_stb') = dummy_test_number)
		// if there is no limit specified then test number will be added in increasing order for each
		// unique parameter.
		map <wstring, int> unique_params;
		int test_number_counter = 1;

		// iteratre through each csv file
		// represents temp structure, where each fieldname is wstring combining
		// unique conditions(e.g. "{cond_vio}{cond_vbat}")
		// internal_json = struct();
		map <wstring, map<wstring, map<wstring, wstring>>> internal_json;
		// keep count of lines in file
		int line_count = 0;

		// get parent folder name for png match
		//wstring curr_file = L"C:\\Users\\XingJin\\Desktop\\matdata.mat";
		wstring curr_file = path_mat_data;
		wstring parent_folder = curr_file.substr(0, curr_file.find_last_of(L"\\") + 1);
		// wcout << "Parent folder: " << parent_folder << endl;
		// conditions that will help to match corresponding png and .mat files for raw_data_link and waveform links
		vector<wstring> file_match_conditions;
		// add first png file match condition to png_file_match_conditions
		file_match_conditions.push_back(parent_folder);

		// start reading file
		// iterate trough all rows of data cell 
		for (int row_index = 0; row_index < row_array_data; row_index++) {
			line_count++;
			cout << "line_count:" << line_count << endl;
			// !!!have to know the type of the current cell : cellArrayData
			cellArrayData = mxGetCell(pMxArrayDataSubset, row_index);

			if (cellArrayData == NULL) {
				int nRows = 0;
				int nCols = 0;
				cout << "Processing data part" << endl;
				// type_indicator_int: indicates whether it is #variables or test data
				type_indicator_int = 1;
			}
			else {
				int nRows = mxGetM(cellArrayData);
				int nCols = mxGetN(cellArrayData);
				cout << "nRows:" << nRows << endl;
				cout << "nCols:" << nCols << endl;
				// type_indicator_int: indicates whether it is #variables or test data
				type_indicator_int = check_data_type(cellArrayData);
			}
			// dimension of the current cell 

			//current row is #variables
			if (type_indicator_int == 2) {
				// read the content of the current cell. return type is wstring
				type_indicator_ws = mat_read_string(cellArrayData);
				//wcout << "String field:" << ws_middle << endl;
				//cout << "address of char:" << static_cast<void *>(&pr) << endl;
				if (type_indicator_ws.find(L"#FIELD") != wstring::npos) {
					field = get_whole_row_test_data(pMxArrayDataSubset, field, row_index, row_array_data, col_array_data);
				}
				else if (type_indicator_ws.find(L"#usl") != wstring::npos) {
					usl = get_whole_row_test_data(pMxArrayDataSubset, usl, row_index, row_array_data, col_array_data);
				}
				else if (type_indicator_ws.find(L"#lsl") != wstring::npos) {
					lsl = get_whole_row_test_data(pMxArrayDataSubset, lsl, row_index, row_array_data, col_array_data);
				}
				else if (type_indicator_ws.find(L"#unit") != wstring::npos) {
					unit_meta = get_whole_row_test_data(pMxArrayDataSubset, unit_meta, row_index, row_array_data, col_array_data);
				}
				else if (type_indicator_ws.find(L"#name") != wstring::npos) {
					name = get_whole_row_test_data(pMxArrayDataSubset, name, row_index, row_array_data, col_array_data);
				}
			}
			//curent row is test data
			else {
				test_data.clear();
				test_data_rows++;
				//curent row is test data
				// vector<wstring> e.g.
				test_data = get_whole_row_test_data(pMxArrayDataSubset, test_data, row_index, row_array_data, col_array_data);

				//keyname is used for tracking the name of the current column( cond + out )
				// key_name wstring (e.g. conv_VIO)
				wstring key_name = L"";
				// init meta data struct to construct meta_data
				map <wstring, wstring> meta_data;
				// wstring containing combination of conditions
				wstring cond_str = L"";
				wstring key_cond_str = L"";
				// Start: scale, unit:might not be used 
				int scale{};
				wstring unit{};
				wstring scaled_value{};
				// End:scale, unit:might not be used 
				vector<wstring> comments{};

				// build cond_ metadata
				for (int current_col = 0; current_col < col_array_data; current_col++) {
					// another test is ignored since assume .mat is better formatted
					// check if param name is not present skip column
					if (name[current_col].empty()) {
						continue;
					}
					// check if current column corresponds to parameter
					if (field[current_col].compare(L"cond") == 0) {
						// construct meta_data key name (e.g. conv_VIO)
						// !!! most important one
						key_name = L"cond_" + name[current_col];
						// handle special cases
						if (convert_to_lower(key_name).compare(L"cond_tambient") == 0) {
							// if temperature is empty, make it 0
							if (test_data[current_col].empty()) {
								wcout << L"TEMP IS EMPTY AT " << row_index << endl;
								test_data[current_col] = L"0";
							}
						}
						else if (key_name.compare(L"cond_vio") == 0) {
							key_name = L"cond_VIO";
						}
						// combine conditions
						cond_str = cond_str + L"_" + test_data[current_col];
						cond_str = cond_str + overall_meta_data[L"username"] + L"_" + overall_meta_data[L"basic_type"] + L"_" + overall_meta_data[L"product_sales_code"] + L"_" + overall_meta_data[L"product_design_step"] + L"_" +
							overall_meta_data[L"package"] + L"_" + overall_meta_data[L"dut_id"];
						// assign value to the right name
						meta_data[key_name] = test_data[current_col];
						// add each condition to the png_file_match_conditions with values. add [ as end of condition (e.g. vio=3[V])
						file_match_conditions.push_back(name[current_col] + L"=" + test_data[current_col] + L"[");
					}
					// check if current column corresponds to comment, except if variable is picture path or waveform path
					if (convert_to_lower(field[current_col]).find(L"comment") != wstring::npos && name[current_col] != L"picture_path" && name[current_col] != L"wfm_path" &&
						!test_data[current_col].empty()) {
						comments.push_back(test_data[current_col]);
					}
				}
				// get cond_link as path to the folder containing current CSV file
				meta_data[L"cond_link_screenshots"] = L"file:///" + strrep(curr_file.substr(0, curr_file.find_last_of(L"\\")), '\\', '/');
				meta_data[L"cond_link_raw_data"] = L"file:///" + strrep(curr_file.substr(0, curr_file.find_last_of(L"\\")), '\\', '/');
				// Start:------------------------- distinguish waveform or data (mat)------------------------
				/*
				// since for now we use only folder name, it doesn't matter how many files matched. All of them are in the same folder
				wstring matching_mat_filename{};
				for (auto mat_file : mat_files) {
				if (mat_file.find(parent_folder) != wstring::npos) {
				matching_mat_filename = mat_file;
				break;
				}
				}
				// constuct proper cond_link_waveforms if matching mat file was found
				if (!matching_mat_filename.empty()) {
				// todo: uncomment this for cond_link_waveforms
				// meta_data[l"cond_link_waveforms"] = l"file:///" + waveform_explorer_path + l" /k " + matching_mat_filename;
				// meta_data[l"cond_link_waveforms"] = strrep(meta_data[l"cond_link_waveforms"], '\\', '/');
				meta_data[l"cond_link_waveforms"] = l"file:///" + strrep(matching_mat_filename.substr(0, matching_mat_filename.find_last_of(l"\\")), '\\', '/');
				}
				*/
				// End:------------------------- distinguish waveform or data (mat)------------------------
				// row_array_data <--> line_count
				repeated_conds[cond_str][curr_file].push_back(row_array_data);

				// iterate through each col again for the aux_sequenceNumber
				// add sequence number for each test case 
				for (int current_col = 0; current_col < col_array_data; current_col++) {
					if (field[current_col].compare(L"aux") == 0 && name[current_col].compare(L"idx") == 0) {
						meta_data[L"idx"] = test_data[current_col];
					}
				}
				// add subset id
				meta_data[L"subset_id"] = ws_id;
				// map <wstring, wstring> payload;

				// iterate through each col again and for each out
				// construct dataObject with payload + meta_data
				for (int current_col = 0; current_col < col_array_data; current_col++) {
					// another test is ignored since assume .mat is better formatted
					// check if param name is not present skip column
					if (name[current_col].empty()) {
						continue;
					}
					if (field[current_col].compare(L"out") == 0 || field[current_col].compare(L"aux") == 0) {
						if (name[current_col].compare(L"idx") == 0)
							continue;
						// skip if empty
						if (test_data[current_col].empty()) {
							continue;
						}
						// init structre to keep payload
						map <wstring, wstring> payload;
						// construct key_name from variables row, e.g. ibat_stb
						key_name = name[current_col];
						// validate key_name
						key_name = validate_param_name(key_name);
						// add out param name to keep param conds str separately
						//keyname is used for tracking the name of the current column 
						key_cond_str = key_name + cond_str;

						scaled_value = test_data[current_col];
						// !!!!!!!!!! add scale value to the variable of payload 
						payload[key_name] = scaled_value;
						// save related png and mat waveforms 
						// if there are matching png files save them to payload
						file_match_conditions.push_back(L"Report-Picture");
						vector<wstring> matching_png_files = get_corresponding_files(file_match_conditions, png_files);
						file_match_conditions.pop_back();

						// save related png files to current payload
						for (auto i = 0; i < matching_png_files.size(); i++) {
							payload[L"png_filename___" + to_wstring(i)] = strrep(matching_png_files[i], '\\', '/');
						}

						// get corresponding .mat files
						file_match_conditions.push_back(L"Report-waveform");
						vector<wstring> matching_mat_files = get_corresponding_files(file_match_conditions, mat_wfm_files);
						file_match_conditions.pop_back();
						// save related .mat files
						for (auto i = 0; i < matching_mat_files.size(); i++) {
							payload[L"mat_filename___" + to_wstring(i)] = strrep(matching_mat_files[i], '\\', '/');
						}
					
						// save related comments
						for (auto i = 0; i < comments.size(); i++) {
							payload[L"comment___" + to_wstring(i)] = comments[i];
						}

						// add other meta fields
						meta_data[L"test_name"] = key_name;
						meta_data[L"data_object_type"] = L"value";
						meta_data[L"dut_id"] = overall_meta_data[L"dut_id"];
						meta_data[L"package"] = overall_meta_data[L"package"];
						meta_data[L"user_name"] = overall_meta_data[L"username"];
						meta_data[L"test_program_name"] = overall_meta_data[L"test_program_name"];
						meta_data[L"test_program_revision"] = overall_meta_data[L"testunit_version"];
						meta_data[L"rddf_tc_id"] = overall_meta_data[L"api_id"] + L":" + overall_meta_data[L"global_id"];
						// !!!!! import parameters limits_struct
						// add test number from limits if it exists, otherwise hardcode

						if (limits_struct.find(key_name) != limits_struct.end()) {
							// get test number from limits
							meta_data[L"test_number"] = limits_struct[key_name][L"TestNr"];
						}
						else {
							// if limit doesn't exist, check if hardcoded test number already exists
							if (unique_params.find(key_name) != unique_params.end()) {
								// use already assigned test number
								meta_data[L"test_number"] = to_wstring(unique_params[key_name]);
							}
							else if (unique_params.empty()) {
								// first unique parameter. Add test number manually and increment test_number_counter
								meta_data[L"test_number"] = to_wstring(test_number_counter);
							}
							else {
								// otherwise assign a new unique test number
								// by incrementing test_counter while uniqueness is achieved
								// to avoid overlap with test numbers from limits file
								bool found_unique = false;
								while (!found_unique) {
									for (map <wstring, int>::value_type& unique_param : unique_params) {
										if (unique_param.second == test_number_counter) {
											found_unique = false;
											break;
										}
										else {
											found_unique = true;
										}
									}
									test_number_counter++;
								}
								meta_data[L"test_number"] = to_wstring(test_number_counter);
							}
						}

						// create dataObject for current out value with payload and meta_data
						map <wstring, map<wstring, wstring>> data_object;
						data_object[L"payload"] = payload;
						data_object[L"metaData"] = meta_data;

						// if key_cond_str is already in internal_json, condition repetition occurred
						// mark flag true to inform user
						if (internal_json.find(key_cond_str) != internal_json.end()) {
							cond_repetition = true;

							vector<wstring> all_keys;
							for (auto const& imap : internal_json)
								all_keys.push_back(imap.first);
							int rep_times = 0;
							for (wstring ele : all_keys) {
								if (ele.find(key_cond_str) != wstring::npos) {
									rep_times += 1;
								}
							}
							cout << rep_times << endl;
							key_cond_str = key_cond_str + L'_rep' + to_wstring(rep_times);
						}

						// store current metaData and payload in internal_json
						internal_json[key_cond_str] = data_object;

						// add structure for limit 594 -- 707
						if (unique_params.find(key_name) == unique_params.end()) {
							// create a payload for current limit
							map <wstring, wstring> limit_payload;
							// create a meta_data for current limit
							map <wstring, wstring> limit_meta_data;
							// define limit_struct to store single limit structure
							map<wstring, wstring> limit_struct;
							if (!lsl.empty() && !usl.empty() && !lsl[current_col].empty() && !usl[current_col].empty()
								&& current_col < lsl.size() && current_col < usl.size()) {
								// get scale, unit
								tie(scale, unit) = dr.get_unit_scale(unit_meta[current_col]);
								// hardcode scale 0, because tembo does auto conversion
								limit_payload[L"scale"] = L"0";
								limit_payload[L"unit"] = unit;

								// deal with no limits: NaN
								// get lower limit
								if (lsl.at(current_col).find(L"NaN") == 0) {
									limit_payload[L"lower_limit"] = L"";
								}
								else {
									scaled_value = dr.scale_value(scale, lsl[current_col]);
									limit_payload[L"lower_limit"] = scaled_value;
								}
								// get upper limit scaled value
								if (usl.at(current_col).find(L"NaN") == 0) {
									limit_payload[L"upper_limit"] = L"";
								}
								else {
									scaled_value = dr.scale_value(scale, usl[current_col]);
									limit_payload[L"upper_limit"] = scaled_value;
								}

								req_id = L"";
								description = L"";
								typical = L"";
								test_number = to_wstring(test_number_counter);
								// wcout << L"Getting from USL: " << usl[current_col] << endl;
							}
							// limits is definded in the limit structure!
							else if (limits_struct.find(key_name) != limits_struct.end()) {
								// get the current limit structure
								for (map<wstring, map<wstring, wstring>>::value_type& iter : limits_struct) {
									if (iter.first == key_name) {
										for (map<wstring, wstring>::value_type& iter_obj : iter.second) {
											limit_struct[iter_obj.first] = iter_obj.second;
										}
									}
								}
								// get scale, unit
								tie(scale, unit) = dr.get_unit_scale(limit_struct[L"Unit"]);
								// hardcode scale 0, because tembo does auto conversion
								limit_payload[L"scale"] = L"0";
								limit_payload[L"unit"] = unit;

								// get lower limit
								scaled_value = dr.scale_value(scale, limit_struct[L"LSL"]);
								limit_payload[L"lower_limit"] = scaled_value;

								// get upper limit scaled value
								scaled_value = dr.scale_value(scale, limit_struct[L"USL"]);
								limit_payload[L"upper_limit"] = scaled_value;

								// add meta data from limit struct
								req_id = limit_struct[L"ReqID"];
								description = limit_struct[L"Description"];
								typical = limit_struct[L"Typ"];
								test_number = limit_struct[L"TestNr"];
							}
							else {
								// use hardcoded limits
								// get scale and unit
								tie(scale, unit) = dr.get_unit_scale(unit_meta[current_col]);
								limit_payload[L"unit"] = unit;
								// hardcode scale to 0, because tembo does auto conversion
								limit_payload[L"scale"] = L"0";

								// get upper limit
								// limit_payload["upper_limit"] = generate_limit_from_test_value(payload[key_name], true);

								// get lower limit
								// limit_payload["lower_limit"] = generate_limit_from_test_value(payload[key_name], false);

								// Back to empty limits
								limit_payload[L"upper_limit"] = L"";
								limit_payload[L"lower_limit"] = L"";

								req_id = L"";
								description = L"";
								typical = L"";
								test_number = to_wstring(test_number_counter);
								// save no matches in txt
								no_limit_match.push_back(key_name);
							}
							// construct limit meta data
							limit_meta_data = dr.construct_limit_meta_data(common_meta_data, req_id, description, typical, test_number, key_name);
							// create a data object for current limit
							map <wstring, map<wstring, wstring>> limit_data_object;
							limit_data_object[L"payload"] = limit_payload;
							limit_data_object[L"metaData"] = limit_meta_data;
							// add limit_data_object to data_objects
							data_objects.push_back(limit_data_object);
							// store unique out params to add limits
							// check if it has defined limits or hard coded
							if (limits_struct.find(key_name) != limits_struct.end() && usl.empty() && lsl.empty()) {
								unique_params[key_name] = stoi(limit_struct[L"TestNr"]);
							}
							else {
								unique_params[key_name] = test_number_counter++;
							}
						}
					}//if (column_types[current_col].compare(L"out") == 0) {
				}//for (int current_col = 0; current_col < test_data.size(); current_col++) {
				 // clear png file match conditions (skip first two for parent folder and dut it)
				while (file_match_conditions.size() > 1) {
					file_match_conditions.pop_back();
				}
			} // Else: curent row is test data

		} // finished reading current mat -> while(inf)
		  // since current csv is done, copy remaining internal json objects into
		  // data_objects, because new file will have different params
		for (map <wstring, map<wstring, map<wstring, wstring>>>::value_type& data_object : internal_json) {
			data_objects.push_back(data_object.second);
		}
	}
	//// create recipe payload
	wstring recipe_payload = construct_recipe(configs_struct[L"ReportTemplate"], configs_struct[L"ReportName"], configs_struct[L"Project"]);
	bool res = dr.json_writer(header_struct, common_meta_data, &data_objects, out_folder_path + L"\\" + configs_struct[L"ReportName"] + L".json", recipe_payload);

	printf("End: Processing Test Data ..................................................\n");
	return res;
}

// will be called within test_data_reader
map <wstring, wstring> configure_reader(DataReader dr, wstring configs_path) {
	printf("Start: Processing Config_Tembo.txt ..................................................\n");
	map <wstring, wstring> configs_struct;
	map<wstring, wstring> raw_configs_struct;
	//wstring wpath{};
	//wpath = L"C:\\svn\\ps_cvsv_projects\\S0000_SVEN\\B11\\20_TestFlow\\Config_Tembo.txt";
	raw_configs_struct = dr.read_config_file(configs_path);
	configs_struct = dr.setup_configurations(raw_configs_struct, true);
	printf("End: Processing Config_Tembo.txt ..................................................\n");
	return configs_struct;
}

//test_data_reader_new for testing purpose
/*
bool test_data_reader_new(mxArray *pMxArrayData, map <wstring, wstring> overall_meta_data, map <wstring, wstring> configs_struct, wstring out_folder_path, wstring path_mat_data, wstring w_out_folder_path, wstring mat_files_first) {
	cout << "entering the test_data_reader_new......" << endl;
	int num_dataset = mxGetN(pMxArrayData);
	mxArray *pMxArrayDataSubset = NULL;
	mxArray *pMxArrayIdSubset = NULL;
	wstring ws_id;
	bool res_data = 0;
	for (int i = 0; i < num_dataset; i++) {
		// iterate through all datasets 
		
		pMxArrayDataSubset = mxGetField(pMxArrayData, i, "data");
		pMxArrayIdSubset = mxGetField(pMxArrayData, i, "id");
		ws_id = mat_read_string(pMxArrayIdSubset);
		cout << "BBBBBBBBBBBBBBBBBBB!!!!!!!!!!!!dimension of the data !!!!!!!!!!BBBBBBBBBBBBBBBBBBBBBB" << endl;
		cout << "dimension of data structure:" << mxGetM(pMxArrayDataSubset) << "___" << mxGetN(pMxArrayDataSubset) << endl;
		wcout << "ws_id: " << ws_id << endl;

		res_data = test_data_reader(pMxArrayDataSubset, overall_meta_data, configs_struct, w_out_folder_path, mat_files_first);

	}

	return res_data;
}
*/

int main(int argc, char *argv[])
{
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::duration<float, std::milli> mil;
	auto t3 = clock::now();
	std::setlocale(LC_ALL, "en_US.utf8");
	//std::locale::global(std::locale("en_US.utf8")); // for C++

	vector<wstring> configs_file;
	map <wstring, wstring> configs_struct;
	map <wstring, wstring> overall_meta_data;
	map <wstring, wstring> meta_data;
	map <wstring, wstring> common_meta_data;

	//wstring configs_path{};
	wstring test_flow_folder{};
	//configs_path = L"C:\\svn\\ps_cvsv_projects\\S0000_SVEN\\B11\\20_TestFlow\\Config_Tembo.txt";
	string path{};
	wstring wpath{};
	//used for right click on a single folder
	wstring searchpath{};
	bool use_sys_pause = true;
	//wstring w_out_folder_path = L"C:\\Users\\XingJin\\Desktop";

	DataReader dr;
	MATFile *pmatFile = NULL;
	mxArray *pMxArrayData = NULL;
	mxArray *pMxArrayMeta = NULL;
	mxArray *pMxArrayInstr = NULL;
	mxArray *pMxArrayLog = NULL;
	mxArray *pMxArrayMeas = NULL;
	mxArray *pMxArraySetup = NULL;
	mxArray *pMxArraysw = NULL;
	//mxArray *cellArrayData = NULL;

	path = argv[1];
	wstring searchPathTmp(path.begin(), path.end());
	searchpath = searchPathTmp;
	// check whether upload whole 30_RawData or just a single folder within it 
	int sign = -1;
	sign = path.find("30_RawData\\");
	// right click on a single file 
	if (sign != -1) {
		path = path.replace(path.find_last_of("\\"), path.size() - 1, "");
	}
	wstring wsTmp(path.begin(), path.end());
	wpath = wsTmp;
	// check if input contains number. If it does remove system pause (another program is calling)
	double doub;
	wistringstream iss(wpath);
	iss >> dec >> doub;
	if (!iss.fail()) {
		use_sys_pause = false;
	}

	vector<wstring> mat_files;
	mat_files = getAllFilesInDir(searchpath, L".mat");
	vector<wstring> png_files;
	vector<wstring> mat_wfm_files;
	// read all png files
	png_files = getAllFilesInDir(searchpath, L".png");
	// read all mat files
	//mat_wfm_files = getAllFilesInDir(searchpath, L".mat");


	// find all folders there 
	vector<wstring> waveformfolder_inside_searchpath;
	for (auto& p : filesys::recursive_directory_iterator(searchpath))
		if (is_directory(p))
			waveformfolder_inside_searchpath.push_back(wstring(p.path()));
	wstring searchpath_waveforms{};
	for (auto folder : waveformfolder_inside_searchpath) {
		if (folder.find(L"waveform")) {
			cout << "found .mat waveforms." << endl;
			searchpath_waveforms = folder;
		}
	}
	// read all mat waveforms within the current folder !!!!! need to change the path level
	mat_wfm_files = getAllFilesInDir(searchpath_waveforms, L".mat");
	for (auto mat_waveform : mat_wfm_files) {
		mat_files.erase(remove(mat_files.begin(), mat_files.end(), mat_waveform), mat_files.end());
	}


	//pmatFile = matOpen("C:\\Users\\XingJin\\Desktop\\matdata.mat", "r");
	// convert wstring to char *. Since matOpen require in this format
	const wchar_t *path_mat_data_wchar = mat_files.at(0).c_str();
	_bstr_t path_mat_data_char(path_mat_data_wchar);
	const char* c = path_mat_data_char;
	pmatFile = matOpen(path_mat_data_char, "r");
	pMxArrayData = matGetVariable(pmatFile, "subsets");
	pMxArrayMeta = matGetVariable(pmatFile, "meta");
	//dimension of the overall data and metadata
	cout << "AAAAAAAAAAAAAAAA!!!!!!!!!!!!dimension of the data !!!!!!!!!!AAAAAAAAAAAAAAAAAAA" << endl;
	cout << "dimension of data structure:" << mxGetM(pMxArrayData) << "___" << mxGetN(pMxArrayData) << endl;

	test_flow_folder = wpath;
	test_flow_folder = test_flow_folder.replace(test_flow_folder.find_last_of(L"\\") + 1, test_flow_folder.size() - 1, L"20_TestFlow");
	configs_file = getAllFilesInDir(test_flow_folder, L"Config_Tembo.txt");
	configs_struct = configure_reader(dr, configs_file[0]);
	// Metadata------------------------------------------------------------------------
	//overall_meta_data = hardcode_overall_metadata();
	// get all necessary metadata from meta 
	overall_meta_data = construct_overall_meta_data(pMxArrayMeta);
	
	// get name of the folder containing csv file -> test_program_name
	wstring test_program_name = mat_files[0].substr(0, mat_files[0].find_last_of(L"\\"));
	test_program_name = test_program_name.substr(test_program_name.find_last_of(L"\\") + 1, test_program_name.size() - 1);
	overall_meta_data[L"test_program_name"] = test_program_name;
	// Data-----------------------------------------------------------------------------
	// get the output folder
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	int day = aTime->tm_mday;
	int month = aTime->tm_mon + 1;
	int year = aTime->tm_year + 1900;
	int hour = aTime->tm_hour;
	int min = aTime->tm_min;
	int sec = aTime->tm_sec;

	bool month_one_digit = false;
	bool day_one_digit = false;
	bool hour_one_digit = false;
	bool min_one_digit = false;
	bool sec_one_digit = false;

	if (month < 10) month_one_digit = true;
	if (day < 10) day_one_digit = true;
	if (hour < 10) hour_one_digit = true;
	if (min < 10) min_one_digit = true;
	if (sec < 10) sec_one_digit = true;

	string out_folder_name = "50_Report\\" + to_string(year) + ((month_one_digit) ? to_string(0) : +"") + to_string(month) + ((day_one_digit) ? to_string(0) : +"") + to_string(day) + "T" + ((hour_one_digit) ? to_string(0) : +"") + to_string(hour) + ((min_one_digit) ? to_string(0) : +"") + to_string(min) + ((sec_one_digit) ? to_string(0) : +"") + to_string(sec);
	// get the output folder path
	string out_folder_path = path.replace(path.find_last_of("\\") + 1, path.size() - 1, out_folder_name);
	wstring wsTmp2(out_folder_path.begin(), out_folder_path.end());
	wstring w_out_folder_path = wsTmp2;

	/*
	if (CreateDirectory(out_folder_path.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
		cout << "succeed in creating output folders!" << endl;
		bool res_data = test_data_reader_new(pMxArrayData, overall_meta_data, configs_struct, w_out_folder_path, mat_files[0], w_out_folder_path, mat_files[0]);
	}
	*/
	
	if (CreateDirectory(out_folder_path.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
		cout << "succeed in creating output folders!" << endl;
		//wstring w_out_folder_path = L"C:\\svn\\ps_cvsv_projects\\S0000_SVEN\\B11\\50_Report\\2021322T1612";
		bool res_data = test_data_reader(pMxArrayData, overall_meta_data, configs_struct, w_out_folder_path, mat_files[0], png_files, mat_wfm_files);

		
		if (res_data) {
			printf("Start: Moving data to staging area ..................................................\n");
			wstring prj_name = configs_struct[L"Project"];
			transform(prj_name.begin(), prj_name.end(), prj_name.begin(), ::toupper);
			wstring staging_area = wstring(L"\\\\VIHSDV002.infineon.com\\tembo_staging_prod\\") + prj_name + L"\\job";
			wcout << L"Staging area location" << endl << staging_area << endl;

			wstring report_name = configs_struct[L"ReportName"];
			// move file to Tembo
			try {
				// move png files
				for (auto png_file : png_files) {
					if (dr.convert_to_lower(png_file).find(L"report-picture") != wstring::npos) {
						filesys::copy(png_file, staging_area, filesys::copy_options::overwrite_existing);
					}
				}
				// move mat files
				for (auto mat_waveform : mat_wfm_files) {
					if (dr.convert_to_lower(mat_waveform).find(L"report-waveform") != wstring::npos) {
						filesys::copy(mat_waveform, staging_area, filesys::copy_options::overwrite_existing);
					}
				}

				filesys::copy(w_out_folder_path + L"\\" + report_name + L".json", staging_area, filesys::copy_options::overwrite_existing);
			}
			catch (filesys::filesystem_error &e) {
				cout << "Couldn't copy file to staging area: " << e.what() << endl;
				wcout << staging_area << endl;
				wcout << w_out_folder_path + L"\\" + report_name + L".json" << endl;
			}
			printf("End: Moving data to staging area ..................................................\n");
		}
		
	}
	else {
		wcout << L"Failed to create directory!" << endl;
	}
	
	auto t4 = clock::now();
	cout << "Total time: " << mil(t4 - t3).count() << " ms" << endl;
	cout << "----------------------------------------------------------" << endl;
	cout << "path of the current file_argv[1]: " << argv[1] << endl;
	cout << "path of the current path: " << path << endl;
	wcout << "path of the current wpath: " << wpath << endl;
	wcout << "path of the current searchpath: " << searchpath << endl;
	cout << "size of the mat file within the search path:" << mat_files.size() << endl;
	wcout << "content of the mat file within the search path:" << mat_files.at(0) << endl;
	wcout << "overall_meta_data[Ltest_program_name]: "<< overall_meta_data[L"test_program_name"] << endl;
	if (configs_file.size() > 0) {
		cout << "configs_file.size():" << configs_file.size() << endl;
	}
	wcout << "w_out_folder_path: " << w_out_folder_path << endl;
	
	system("pause");
	matClose(pmatFile);

	return 0;
}