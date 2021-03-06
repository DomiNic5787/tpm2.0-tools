/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <tss2/tss2_esys.h>

#include "log.h"
#include "tpm2.h"
#include "tpm2_auth_util.h"
#include "tpm2_hierarchy.h"
#include "tpm2_util.h"

/**
 * Parses a hierarchy value from an option argument.
 * @param value
 *  The string to parse, which can be a numerical string as
 *  understood by strtoul() with a base of 0, or an:
 *    - o - Owner hierarchy
 *    - p - Platform hierarchy
 *    - e - Endorsement hierarchy
 *    - n - Null hierarchy
 * @param hierarchy
 *  The parsed hierarchy as output.
 * @param flags
 *  What hierarchies should be supported by
 *  the parsing.
 * @return
 *  True on success, False otherwise.
 */
bool tpm2_hierarchy_from_optarg(const char *value,
        TPMI_RH_PROVISION *hierarchy, tpm2_hierarchy_flags flags) {

    if (!value || !value[0]) {
        return false;
    }

    *hierarchy = 0;

    bool is_o = !strncmp(value, "owner", strlen(value));
    if (is_o) {
        *hierarchy = TPM2_RH_OWNER;
    }

    bool is_p = !strncmp(value, "platform", strlen(value));
    if (is_p) {
        *hierarchy = TPM2_RH_PLATFORM;
    }

    bool is_e = !strncmp(value, "endorsement", strlen(value));
    if (is_e) {
        *hierarchy = TPM2_RH_ENDORSEMENT;
    }

    bool is_n = !strncmp(value, "null", strlen(value));
    if (is_n) {
        *hierarchy = TPM2_RH_NULL;
    }

    bool is_l = !strncmp(value, "lockout", strlen(value));
    if (is_l) {
        *hierarchy = TPM2_RH_LOCKOUT;
    }

    bool result = true;
    if (!*hierarchy) {
        /*
         * This branch is executed when hierarchy is specified as a hex handle.
         * The raw hex returned may be a generic (non hierarchy) TPM2_HANDLE.
         */
        result = tpm2_util_string_to_uint32(value, hierarchy);
    }
    if (!result) {
        LOG_ERR("Incorrect handle value, got: \"%s\", expected [o|p|e|n|l]"
                "or a handle number", value);
        return false;
    }

    /*
     * If the caller specifies the expected valid hierarchies, either as string,
     * or hex handles, they are additionally filtered here.
     */
    if (!(flags & TPM2_HIERARCHY_FLAGS_O) && *hierarchy == TPM2_RH_OWNER) {
            LOG_ERR("Owner hierarchy not supported by this command.");
            return false;
        }

    if (!(flags & TPM2_HIERARCHY_FLAGS_P) && *hierarchy == TPM2_RH_PLATFORM) {
            LOG_ERR("Platform hierarchy not supported by this command.");
            return false;
        }

    if (!(flags & TPM2_HIERARCHY_FLAGS_E) && *hierarchy == TPM2_RH_ENDORSEMENT) {
            LOG_ERR("Endorsement hierarchy not supported by this command.");
            return false;
        }

    if (!(flags & TPM2_HIERARCHY_FLAGS_N) && *hierarchy == TPM2_RH_NULL) {
            LOG_ERR("NULL hierarchy not supported by this command.");
            return false;
        }

    if (!(flags & TPM2_HIERARCHY_FLAGS_L) && *hierarchy == TPM2_RH_LOCKOUT) {
            LOG_ERR("Permanent handle lockout not supported by this command.");
            return false;
        }

    return result;
}

tool_rc tpm2_hierarchy_create_primary(ESYS_CONTEXT *ectx,
        tpm2_session *sess,
        tpm2_hierarchy_pdata *objdata) {

    ESYS_TR hierarchy;

    hierarchy = tpm2_tpmi_hierarchy_to_esys_tr(objdata->in.hierarchy);

    ESYS_TR shandle1 = ESYS_TR_NONE;
    tool_rc rc = tpm2_auth_util_get_shandle(ectx, hierarchy, sess, &shandle1);
    if (rc != tool_rc_success) {
        LOG_ERR("Couldn't get shandle for hierarchy");
        return rc;
    }

    return tpm2_create_primary(ectx, hierarchy,
            shandle1, ESYS_TR_NONE, ESYS_TR_NONE,
            &objdata->in.sensitive, &objdata->in.public,
            &objdata->in.outside_info, &objdata->in.creation_pcr,
            &objdata->out.handle, &objdata->out.public,
            &objdata->out.creation.data, &objdata->out.hash,
            &objdata->out.creation.ticket);
}

void tpm2_hierarchy_pdata_free(tpm2_hierarchy_pdata *objdata) {

    free(objdata->out.creation.data);
    objdata->out.creation.data = NULL;
    free(objdata->out.creation.ticket);
    objdata->out.creation.ticket = NULL;
    free(objdata->out.hash);
    objdata->out.hash = NULL;
    free(objdata->out.public);
    objdata->out.public = NULL;
}
