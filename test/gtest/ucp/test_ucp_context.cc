/**
* Copyright (C) Mellanox Technologies Ltd. 2001-2015.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include "ucp_test.h"


class test_ucp_context : public ucp_test {
public:
    static ucp_params_t get_ctx_params() {
        ucp_params_t params = ucp_test::get_ctx_params();
        params.features |= UCP_FEATURE_TAG | UCP_FEATURE_WAKEUP;
        return params;
    }
};


class test_ucp_aliases : public test_ucp_context {
public:
    using test_ucp_context::get_ctx_params;
};

UCS_TEST_P(test_ucp_aliases, aliases) {
    create_entity();
}

UCP_INSTANTIATE_TEST_CASE_TLS(test_ucp_aliases, rc, "rc")
UCP_INSTANTIATE_TEST_CASE_TLS(test_ucp_aliases, rc_x, "rc_x")
UCP_INSTANTIATE_TEST_CASE_TLS(test_ucp_aliases, ud, "ud")
UCP_INSTANTIATE_TEST_CASE_TLS(test_ucp_aliases, ud_mlx5, "ud_mlx5")
UCP_INSTANTIATE_TEST_CASE_TLS(test_ucp_aliases, ugni, "ugni")
UCP_INSTANTIATE_TEST_CASE_TLS(test_ucp_aliases, shm, "shm")


class test_ucp_version : public test_ucp_context {
public:
    using test_ucp_context::get_ctx_params;
};

UCS_TEST_P(test_ucp_version, wrong_api_version) {

    ucs::handle<ucp_config_t*> config;
    UCS_TEST_CREATE_HANDLE(ucp_config_t*, config, ucp_config_release,
                           ucp_config_read, NULL, NULL);

    ucp_params_t params = get_ctx_params();
    ucp_context_h ucph;
    ucs_status_t status;
    {
        disable_errors();
        status = ucp_init_version(99, 99, &params, config.get(), &ucph);
        restore_errors();
    }
    if (status == UCS_OK) {
        ucp_cleanup(ucph);
        ADD_FAILURE() << "Created UCP with wrong version";
    }
}

UCS_TEST_P(test_ucp_version, version_string) {

    unsigned major_version, minor_version, release_number;

    ucp_get_version(&major_version, &minor_version, &release_number);

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%d.%d.%d", major_version, minor_version,
             release_number);

    EXPECT_EQ(std::string(buffer), std::string(ucp_get_version_string()));
}

UCP_INSTANTIATE_TEST_CASE_TLS(test_ucp_version, all, "all")
