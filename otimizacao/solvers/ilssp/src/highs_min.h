// Minimal subset of the HiGHS C API (v1.15, 32-bit HighsInt), declared here so
// we can link directly against the libhighs.so shipped inside highspy.
#pragma once

#include <cstdint>

typedef int32_t HighsInt;

extern "C" {
void* Highs_create(void);
void Highs_destroy(void* highs);
HighsInt Highs_getSizeofHighsInt(const void* highs);
HighsInt Highs_passMip(void* highs, const HighsInt num_col, const HighsInt num_row,
                       const HighsInt num_nz, const HighsInt a_format, const HighsInt sense,
                       const double offset, const double* col_cost, const double* col_lower,
                       const double* col_upper, const double* row_lower, const double* row_upper,
                       const HighsInt* a_start, const HighsInt* a_index, const double* a_value,
                       const HighsInt* integrality);
HighsInt Highs_passLp(void* highs, const HighsInt num_col, const HighsInt num_row,
                      const HighsInt num_nz, const HighsInt a_format, const HighsInt sense,
                      const double offset, const double* col_cost, const double* col_lower,
                      const double* col_upper, const double* row_lower, const double* row_upper,
                      const HighsInt* a_start, const HighsInt* a_index, const double* a_value);
HighsInt Highs_run(void* highs);
HighsInt Highs_getSolution(const void* highs, double* col_value, double* col_dual,
                           double* row_value, double* row_dual);
HighsInt Highs_getModelStatus(const void* highs);
double Highs_getObjectiveValue(const void* highs);
HighsInt Highs_setBoolOptionValue(void* highs, const char* option, const HighsInt value);
HighsInt Highs_setIntOptionValue(void* highs, const char* option, const HighsInt value);
HighsInt Highs_setDoubleOptionValue(void* highs, const char* option, const double value);
HighsInt Highs_setStringOptionValue(void* highs, const char* option, const char* value);
HighsInt Highs_setSolution(void* highs, const double* col_value, const double* row_value,
                           const double* col_dual, const double* row_dual);
HighsInt Highs_getIntInfoValue(const void* highs, const char* info, HighsInt* value);
HighsInt Highs_getDoubleInfoValue(const void* highs, const char* info, double* value);
}

constexpr HighsInt kHighsMatrixFormatColwise = 1;
constexpr HighsInt kHighsObjSenseMinimize = 1;
constexpr HighsInt kHighsVarTypeInteger = 1;
constexpr HighsInt kHighsModelStatusOptimal = 7;
constexpr HighsInt kHighsModelStatusTimeLimit = 13;
