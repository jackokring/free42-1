/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <stdlib.h>

#include "core_commands2.h"
#include "core_commands5.h"
#include "core_commands7.h"
#include "core_display.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math1.h"
#include "core_sto_rcl.h"
#include "core_variables.h"
#include "core_linalg1.h"

/********************************************************/
/* Implementations of HP-42S built-in functions, part 5 */
/********************************************************/

int appmenu_exitcallback_2(int menuid, bool exitall) {
    if (menuid >= MENU_BASE
            && menuid <= MENU_BASE_LOGIC
            && !exitall) {
        mode_appmenu = menuid;
        set_appmenu_exitcallback(2);
    } else {
        set_base(10);
        mode_appmenu = menuid;
        baseapp = 0;
    }
    return ERR_NONE;
}

static int base_helper(int base) {
    if (program_running()) {
        int err = set_menu_return_err(MENULEVEL_APP, MENU_BASE, false);
        if (err != ERR_NONE)
            return err;
        set_appmenu_exitcallback(2);
        baseapp = 1;
    }
    set_base(base);
    return ERR_NONE;
}

int docmd_binm(arg_struct *arg) {
    return base_helper(2);
}

int docmd_octm(arg_struct *arg) {
    return base_helper(8);
}

int docmd_decm(arg_struct *arg) {
    return base_helper(10);
}

int docmd_hexm(arg_struct *arg) {
    return base_helper(16);
}

int docmd_linf(arg_struct *arg) {
    flags.f.lin_fit = 1;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_logf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 1;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_expf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 1;
    flags.f.pwr_fit = 0;
    return ERR_NONE;
}

int docmd_pwrf(arg_struct *arg) {
    flags.f.lin_fit = 0;
    flags.f.log_fit = 0;
    flags.f.exp_fit = 0;
    flags.f.pwr_fit = 1;
    return ERR_NONE;
}

int docmd_qf(arg_struct *arg) {
    if(flags.f.all_sigma && flags.f.q_sigma)
        flags.f.q_fit = !flags.f.q_fit;
    return ERR_NONE;
}

int docmd_allsigma(arg_struct *arg) {
    flags.f.all_sigma = 1;
    flags.f.q_sigma = 0;
    return ERR_NONE;
}

int docmd_qsigma(arg_struct *arg) {
    if(flags.f.all_sigma)
        flags.f.q_sigma = !flags.f.q_sigma;
    return ERR_NONE;
}

int docmd_and(arg_struct *arg) {
    int8 x, y;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    int8 res = x & y;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_baseadd(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    res = x + y;
    if (!flags.f.base_wrap && effective_wsize() == 64) {
        if (flags.f.base_signed) {
            if (x > 0 && y > 0) {
                if (res < x || res < y)
                    if (flags.f.range_error_ignore)
                        res = (1LL << (effective_wsize() - 1)) - 1;
                    else
                        return ERR_OUT_OF_RANGE;
            } else if (x < 0 && y < 0) {
                if (res > x || res > y)
                    if (flags.f.range_error_ignore)
                        res = -1LL << (effective_wsize() - 1);
                    else
                        return ERR_OUT_OF_RANGE;
            }
        } else {
            if ((uint8) res < (uint8) x || (uint8) res < (uint8) y)
                if (flags.f.range_error_ignore)
                    res = ~0LL;
                else
                    return ERR_OUT_OF_RANGE;
        }
    } else {
        err = base_range_check(&res, false);
        if (err != ERR_NONE)
            return err;
    }
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basesub(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    if (!flags.f.base_signed && !flags.f.base_wrap && (uint8) x > (uint8) y) {
        if (flags.f.range_error_ignore)
            res = 0;
        else
            return ERR_OUT_OF_RANGE;
    } else {
        res = y - x;
        if (!flags.f.base_wrap && effective_wsize() == 64) {
            if (flags.f.base_signed) {
                if (x < 0 && y > 0) {
                    if (x == (int8) 0x8000000000000000LL || res < -x || res < y)
                        if (flags.f.range_error_ignore)
                            res = (1LL << (effective_wsize() - 1)) - 1;
                        else
                            return ERR_OUT_OF_RANGE;
                } else if (x > 0 && y < 0) {
                    if (y == (int8) 0x8000000000000000LL || res > -x || res > y)
                        if (flags.f.range_error_ignore)
                            res = -1LL << (effective_wsize() - 1);
                        else
                            return ERR_OUT_OF_RANGE;
                }
            }
        } else {
            err = base_range_check(&res, false);
            if (err != ERR_NONE)
                return err;
        }
    }
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basemul(arg_struct *arg) {
    int8 x, y;
    int8 res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    if (x == 0 || y == 0) {
        res = 0;
    } else {
        /* Performing a 64-bit x 64-bit => 128-bit multiplication.
         * Doing it unsigned to keep things somewhat simple.
         */
        int wsize = effective_wsize();
        bool neg = false;
        uint8 op1, op2;
        if (flags.f.base_signed) {
            if (wsize == 64) {
                if (x == (int8) 0x8000000000000000LL && y != 1) {
                    if (flags.f.base_wrap)
                        res = (y & 1) == 0 ? 0 : x;
                    else if (flags.f.range_error_ignore)
                        res = y > 0 ? 0x8000000000000000LL : 0x7fffffffffffffffLL;
                    else
                        return ERR_OUT_OF_RANGE;
                } else if (y == (int8) 0x8000000000000000LL && x != 1) {
                    if (flags.f.base_wrap)
                        res = (x & 1) == 0 ? 0 : y;
                    else if (flags.f.range_error_ignore)
                        res = x > 0 ? 0x8000000000000000LL : 0x7fffffffffffffffLL;
                    else
                        return ERR_OUT_OF_RANGE;
                }
            }
            if (x < 0) {
                neg = true;
                op1 = (uint8) -x;
            } else {
                op1 = (uint8) x;
            }
            if (y < 0) {
                neg = !neg;
                op2 = (uint8) -y;
            } else {
                op2 = (uint8) y;
            }
        } else {
            op1 = (uint8) x;
            op2 = (uint8) y;
        }

        uint8 u1 = op1 & 0xffffffff;
        uint8 v1 = op2 & 0xffffffff;
        uint8 t = u1 * v1;
        uint8 w3 = t & 0xffffffff;
        uint8 k = t >> 32;

        op1 >>= 32;
        t = op1 * v1 + k;
        k = t & 0xffffffff;
        uint8 w1 = t >> 32;

        op2 >>= 32;
        t = u1 * op2 + k;
        k = t >> 32;

        uint8 hi = op1 * op2 + w1 + k;
        uint8 lo = (t << 32) + w3;

        if (flags.f.base_wrap) {
            if (neg)
                res = -(int8) lo;
            else
                res = (int8) lo;
            base_range_check(&res, true);
        } else if (flags.f.base_signed) {
            if (neg) {
                if (hi != 0 || lo > (1ULL << (wsize - 1)))
                    if (flags.f.range_error_ignore)
                        lo = 1ULL << (wsize - 1);
                    else
                        return ERR_OUT_OF_RANGE;
                res = - (int8) lo;
            } else {
                if (hi != 0 || lo > (1ULL << (wsize - 1)) - 1)
                    if (flags.f.range_error_ignore)
                        lo = (1ULL << (wsize - 1)) - 1;
                    else
                        return ERR_OUT_OF_RANGE;
                res = (int8) lo;
            }
        } else {
            if (hi != 0 || wsize < 64 && lo > (1ULL << wsize) - 1)
                if (flags.f.range_error_ignore)
                    lo = wsize == 64 ? ~0ULL : (1ULL << wsize) - 1;
                else
                    return ERR_OUT_OF_RANGE;
            res = (int8) lo;
        }
    }

    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basediv(arg_struct *arg) {
    int8 x, y, res;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    if (x == 0)
        return ERR_DIVIDE_BY_0;
    if (flags.f.base_signed)
        res = y / x;
    else
        res = (int8) (((uint8) y) / ((uint8) x));
    if ((err = base_range_check(&res, false)) != ERR_NONE)
        return err;
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_basechs(arg_struct *arg) {
    int8 x;
    int err;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    if (flags.f.base_wrap) {
        x = -x;
        base_range_check(&x, true);
    } else if (flags.f.base_signed) {
        int8 maxneg = -1LL << (effective_wsize() - 1);
        if (x == maxneg) {
            if (flags.f.range_error_ignore)
                x = -1 - maxneg;
            else
                return ERR_OUT_OF_RANGE;
        } else
            x = -x;
    } else {
        if (x != 0) {
            if (flags.f.range_error_ignore)
                x = 0;
            else
                return ERR_OUT_OF_RANGE;
        } else
            x = 0;
    }
    ((vartype_real *) reg_x)->x = base2phloat(x);
    return ERR_NONE;
}

static struct sum_struct {
    phloat x;
    phloat x2;
    phloat y;
    phloat y2;
    phloat xy;
    phloat n;
    phloat lnx;
    phloat lnx2;
    phloat lny;
    phloat lny2;
    phloat lnxlny;
    phloat xlny;
    phloat ylnx;
    phloat x2y;
    /* quad order */
    phloat x3;
    phloat x4;
    phloat lnx3;
    phloat lnx4;
    phloat ylnx2;
    phloat x2lny;
    phloat lnx2lny;
} sum;

static int get_summation() {
    /* Check if summation registers are OK */
    int4 first = mode_sigma_reg;
    int4 last = first + (flags.f.all_sigma ? 14 : 6);
    int4 size, i;
    vartype *regs = recall_var("REGS", 4);
    vartype_realmatrix *r;
    phloat *sigmaregs;
    if (regs == NULL)
        return ERR_SIZE_ERROR;
    if (regs->type != TYPE_REALMATRIX)
        return ERR_INVALID_TYPE;
    r = (vartype_realmatrix *) regs;
    size = r->rows * r->columns;
    if (last > size)
        return ERR_SIZE_ERROR;
    for (i = first; i < last; i++)
        if (r->array->is_string[i])
            return ERR_ALPHA_DATA_IS_INVALID;
    sigmaregs = r->array->data + first;
    sum.x = sigmaregs[0];
    sum.x2 = sigmaregs[1];
    sum.y = sigmaregs[2];
    sum.y2 = sigmaregs[3];
    sum.xy = sigmaregs[4];
    sum.n = sigmaregs[5];
    if (flags.f.all_sigma) {
        sum.lnx = sigmaregs[6];
        sum.lnx2 = sigmaregs[7];
        sum.lny = sigmaregs[8];
        sum.lny2 = sigmaregs[9];
        sum.lnxlny = sigmaregs[10];
        sum.xlny = sigmaregs[11];
        sum.ylnx = sigmaregs[12];
        sum.x2y = sigmaregs[13];
        if(flags.f.q_sigma) {
            sum.x3 = sigmaregs[14];
            sum.x4 = sigmaregs[15];
            sum.lnx3 = sigmaregs[16];
            sum.lnx4 = sigmaregs[17];
            sum.ylnx2 = sigmaregs[18];
            sum.x2lny = sigmaregs[19];
            sum.lnx2lny = sigmaregs[20];
        }
    }
    return ERR_NONE;
}
    
static struct model_struct {
    phloat x;
    phloat x2;
    phloat y;
    phloat y2;
    phloat xy;
    phloat n;
    int ln_before;
    int exp_after;
    int valid;
    phloat slope;
    phloat yint;
    /* quad order */
    phloat x2y;
    phloat x3;
    phloat x4;
    phloat quad;
    phloat qslope;//as slope sets best x estimate based on local r
    phloat qyint;//makes easier logic
} model;

#define MODEL_NONE -1
#define MODEL_LIN 0
#define MODEL_LOG 1
#define MODEL_EXP 2
#define MODEL_PWR 3

static void matrix_helper2(int error, vartype *result) {
    if(error != ERR_NONE) return;
    vartype_realmatrix *r = (vartype_realmatrix *) result;
    phloat *data = r->array->data;
    model.quad = data[0];
    model.qslope = data[1];
    model.quad = data[2];
    free_vartype(result);
}

static int matrix_helper1() {
    // caller must call get_model_summation(modl)
    vartype *sig = new_realmatrix(3, 3);
    if(sig == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    vartype *col = new_realmatrix(1, 3);
    if(col == NULL) {
        free_vartype(sig);
        return ERR_INSUFFICIENT_MEMORY;
    }
    vartype_realmatrix *r = (vartype_realmatrix *) sig;
    phloat *data = r->array->data;
    data[0] = model.x4;
    data[1] = model.x3;
    data[2] = model.x2;
    data[3] = model.x3;
    data[4] = model.x2;
    data[5] = model.x;
    data[6] = model.x2;
    data[7] = model.x;
    data[8] = model.n;
    r = (vartype_realmatrix *) col;
    data = r->array->data;
    data[0] = model.x2y;
    data[1] = model.xy;
    data[2] = model.y;
    /* int linalg_div(const vartype *left, const vartype *right,
                             void (*completion)(int, vartype *)); */
    int err = linalg_div(col, sig, matrix_helper2);
    free_vartype(sig);
    free_vartype(col);
    return err;
}

static int get_model_summation(int modl) {
    int err = get_summation();
    if (err != ERR_NONE)
        return err;
    err = (flags.f.q_fit && flags.f.q_sigma
                && model.n > 2) ? matrix_helper1() : ERR_NONE;//before
    switch (modl) {
        case MODEL_LIN:
            model.xy = sum.xy;
            model.ln_before = 0;
            model.exp_after = 0;
            if(flags.f.q_sigma) {
                model.x2y = sum.x2y;
                model.x3 = sum.x3;
                model.x4 = sum.x4;
            }
            break;
        case MODEL_LOG:
            if (flags.f.log_fit_invalid)
                return ERR_INVALID_FORECAST_MODEL;
            model.xy = sum.ylnx;
            model.ln_before = 1;
            model.exp_after = 0;
            if(flags.f.q_sigma) {
                model.x2y = sum.ylnx2;
                model.x3 = sum.lnx3;
                model.x4 = sum.lnx4;
            }
            break;
        case MODEL_EXP:
            if (flags.f.exp_fit_invalid)
                return ERR_INVALID_FORECAST_MODEL;
            model.xy = sum.xlny;
            model.ln_before = 0;
            model.exp_after = 1;
            if(flags.f.q_sigma) {
                model.x2y = sum.x2lny;
                model.x3 = sum.x3;
                model.x4 = sum.x4;
            }
            break;
        case MODEL_PWR:
            if (flags.f.pwr_fit_invalid)
                return ERR_INVALID_FORECAST_MODEL;
            model.xy = sum.lnxlny;
            model.ln_before = 1;
            model.exp_after = 1;
            if(flags.f.q_sigma) {
                model.x2y = sum.lnx2lny;
                model.x3 = sum.lnx3;
                model.x4 = sum.lnx4;
            }
            break;
        default:
            return ERR_INVALID_FORECAST_MODEL;
    }
    if (model.ln_before) {
        model.x = sum.lnx;
        model.x2 = sum.lnx2;
    } else {
        model.x = sum.x;
        model.x2 = sum.x2;
    }
    if (model.exp_after) {
        model.y = sum.lny;
        model.y2 = sum.lny2;
    } else {
        model.y = sum.y;
        model.y2 = sum.y2;
    }
    model.n = sum.n;
    return err;
}

static int corr_helper(int modl, phloat *r) {
    phloat cov, varx, vary, v, tr;
    int err = get_model_summation(modl);
    if (err != ERR_NONE)
        return err;
    if (model.n == 0 || model.n == 1)
        return ERR_STAT_MATH_ERROR;
    if(flags.f.q_fit && flags.f.q_sigma) {//in context of LIN-Sigma
        if (model.n == 2)
            return ERR_STAT_MATH_ERROR;
        /* y2 - 2ax2y - bxy - 2cy + a2x4 + 2abx3 + 2acx2 - bxy + b2x2 + 2bcx - c2 */
        v = model.y2 - 2 * (model.quad * model.x2y - model.qyint
                                                * (model.y - model.qslope * model.x))
                    - model.qslope * model.x * model.y + model.quad * model.quad * model.x4
                    + 2 * (model.quad * model.qslope * model.x3
                                                + model.quad * model.qyint * model.x2)
                    - model.qslope * model.xy + model.qslope * model.qslope * model.x2
                                                - model.qyint * model.qyint;
        /* y2 - 2yMy - (My)2 */
        tr = model.y2 - (2 + 1 / model.n) * model.y * model.y / model.n;
        if(tr == 0) return ERR_STAT_MATH_ERROR;
        tr = 1 - v / tr;
        if(tr < 0) tr = 0;
    } else {
        cov = model.xy - model.x * model.y / model.n;
        varx = model.x2 - model.x * model.x / model.n;
        vary = model.y2 - model.y * model.y / model.n;
        if (varx <= 0 || vary <= 0)
            return ERR_STAT_MATH_ERROR;
        v = varx * vary;
        if (v == 0)
            return ERR_STAT_MATH_ERROR;
        tr = cov / sqrt(v);
    }
    if (tr < -1)
        tr = -1;
    else if (tr > 1)
        tr = 1;
    *r = tr;
    return ERR_NONE;
}

static int slope_yint_helper() {
    /* The caller should have made sure that 'model' is up to date
     * by calling get_model_summation() first.
     */
    phloat cov, varx, meanx, meany;
    int inf;
    if (model.n == 0 || model.n == 1)
        return ERR_STAT_MATH_ERROR;
    cov = model.xy - model.x * model.y / model.n;
    varx = model.x2 - model.x * model.x / model.n;
    if (varx == 0)
        return ERR_STAT_MATH_ERROR;
    model.slope = cov / varx;
    if ((inf = p_isinf(model.slope)) != 0)
        model.slope = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    meanx = model.x / model.n;
    meany = model.y / model.n;
    model.yint = meany - model.slope * meanx;
    if ((inf = p_isinf(model.yint)) != 0)
        model.yint = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    return ERR_NONE;
}

static int get_model() {
    if (flags.f.lin_fit)
        return MODEL_LIN;
    else if (flags.f.log_fit)
        return MODEL_LOG;
    else if (flags.f.exp_fit)
        return MODEL_EXP;
    else if (flags.f.pwr_fit)
        return MODEL_PWR;
    else
        return MODEL_NONE;
}

int docmd_best(arg_struct *arg) {
    int best = MODEL_NONE;
    phloat bestr = 0;
    int firsterr = ERR_NONE;
    int i;
    for (i = MODEL_LIN; i <= MODEL_PWR; i++) {
        phloat r;
        int err = corr_helper(i, &r);
        if (err == ERR_NONE) {
            if (r < 0)
                r = -r;
            if (r > bestr) {
                best = i;
                bestr = r;
            }
        } else {
            if (firsterr == ERR_NONE)
                firsterr = err;
        }
    }
    if (best == MODEL_NONE)
        best = MODEL_LIN;
    else
        firsterr = ERR_NONE;
    flags.f.lin_fit = best == MODEL_LIN;
    flags.f.log_fit = best == MODEL_LOG;
    flags.f.exp_fit = best == MODEL_EXP;
    flags.f.pwr_fit = best == MODEL_PWR;
    return firsterr;
}

int docmd_bit_t(arg_struct *arg) {
    int8 x, y;
    int err;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    if (x < 0 || x >= effective_wsize())
        return ERR_INVALID_DATA;
    return (y & (1ULL << x)) != 0 ? ERR_YES : ERR_NO;
}

int docmd_corr(arg_struct *arg) {
    phloat r;
    int err;
    vartype *rv;
    err = corr_helper(get_model(), &r);
    if (err != ERR_NONE)
        return err;
    rv = new_real(r);
    if (rv == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(rv);
    return ERR_NONE;
}

static int mappable_fcstx(phloat x, phloat *y) {
    int inf;
    if (model.exp_after) {
        if (x <= 0)
            return ERR_INVALID_FORECAST_MODEL;
        x = log(x);
    }
    if(flags.f.q_fit && flags.f.q_sigma) {
        phloat r1;
        r1 = model.qslope * model.qslope - 4 * model.quad * (model.qyint - x);
        if (model.quad == 0 || r1 < 0)
            return ERR_STAT_MATH_ERROR;
        r1 = sqrt(r1);
        /* choice logic */
        if(model.quad * model.slope > 0) {//positive if max root
            x = (-model.qslope + r1) / (2 * model.quad);
        } else {
            x = (-model.qslope - r1) / (2 * model.quad);
        }
    } else {
        if (model.slope == 0)
            return ERR_STAT_MATH_ERROR;
        x = (x - model.yint) / model.slope;
    }
    if (model.ln_before)
        x = exp(x);
    if ((inf = p_isinf(x)) != 0)
        x = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    *y = x;
    return ERR_NONE;
}

int docmd_fcstx(arg_struct *arg) {
    int err = get_model_summation(get_model());
    vartype *v;
    if (err != ERR_NONE)
        return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
        return err;
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
        err = map_unary(reg_x, &v, mappable_fcstx, NULL);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

static int mappable_fcsty(phloat x, phloat *y) {
    int inf;
    if (model.ln_before) {
        if (x <= 0)
            return ERR_INVALID_FORECAST_MODEL;
        x = log(x);
    }
    if(flags.f.q_fit && flags.f.q_sigma) {
        x = x * x * model.quad + x * model.qslope + model.qyint;
    } else {
        x = x * model.slope + model.yint;
    }
    if (model.exp_after)
        x = exp(x);
    if ((inf = p_isinf(x)) != 0)
        x = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    *y = x;
    return ERR_NONE;
}

int docmd_fcsty(arg_struct *arg) {
    int err = get_model_summation(get_model());
    vartype *v;
    if (err != ERR_NONE)
        return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
        return err;
    if (reg_x->type == TYPE_REAL || reg_x->type == TYPE_REALMATRIX) {
        err = map_unary(reg_x, &v, mappable_fcsty, NULL);
        if (err == ERR_NONE)
            unary_result(v);
        return err;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_mean(arg_struct *arg) {
    phloat m;
    int inf;
    vartype *mx, *my;
    int err = get_summation();
    if (err != ERR_NONE)
        return err;
    if (sum.n == 0)
        return ERR_STAT_MATH_ERROR;
    m = sum.x / sum.n;
    if ((inf = p_isinf(m)) != 0)
        m = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    mx = new_real(m);
    if (mx == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    m = sum.y / sum.n;
    if ((inf = p_isinf(m)) != 0)
        m = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    my = new_real(m);
    if (my == NULL) {
        free_vartype(mx);
        return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_y);
    reg_y = my;
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    reg_x = mx;
    if (flags.f.trace_print && flags.f.printer_enable)
        docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_sdev(arg_struct *arg) {
    int err = get_summation();
    phloat var;
    vartype *sx, *sy;
    if (err != ERR_NONE)
        return err;
    if (sum.n == 0 || sum.n == 1)
        return ERR_STAT_MATH_ERROR;
    var = (sum.x2 - (sum.x * sum.x / sum.n)) / (sum.n - 1);
    if (var < 0)
        return ERR_STAT_MATH_ERROR;
    if (p_isinf(var))
        sx = new_real(POS_HUGE_PHLOAT);
    else
        sx = new_real(sqrt(var));
    if (sx == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    var = (sum.y2 - (sum.y * sum.y / sum.n)) / (sum.n - 1);
    if (var < 0)
        return ERR_STAT_MATH_ERROR;
    if (p_isinf(var))
        sy = new_real(POS_HUGE_PHLOAT);
    else
        sy = new_real(sqrt(var));
    if (sy == NULL) {
        free_vartype(sx);
        return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_y);
    reg_y = sy;
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    reg_x = sx;
    if (flags.f.trace_print && flags.f.printer_enable)
        docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_sddx(arg_struct *arg) {
    phloat sd;
    vartype *v;
    int err = get_summation();
    if (err != ERR_NONE)
        return err;
    sd = (sum.n - 1);
    if (sd == 0 || !flags.f.all_sigma)
        return ERR_STAT_MATH_ERROR;
    sd = (sum.x2 - (sum.x * sum.x / sum.n)) / sd;
    if (sd < 0 || sum.n == 0)
        return ERR_STAT_MATH_ERROR;
    sd *= sd;//s4
    sd *= (sum.n - 3) * (sum.n - 1);
    phloat sn = sum.n;
    phloat m4 = (sum.x4 / sn);
    sn *= sn;
    sd /= sn;//get sd part
    m4 -= 4 * sum.x * sum.x3 / sn;
    sn *= sum.n;
    phloat m2 = sum.x * sum.x;
    m4 += 6 * m2 * sum.x2 / sn;
    sn *= sum.n;
    m2 *= m2;
    m4 -= 3 * m2 / sn;//mu sub 4
    m4 -= sd;
    m4 /= sum.n;//Var(s^2)  
    if (m4 < 0)
        return ERR_STAT_MATH_ERROR;
    if (p_isinf(m4))
        v = new_real(POS_HUGE_PHLOAT);
    else
        v = new_real(sqrt(m4));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_rcsd(arg_struct *arg) {
    phloat c;
    vartype *v;
    int err = get_summation();
    if (err != ERR_NONE)
        return err;
    c = (sum.n - 1) / 2;
    if (c < 0)
        return ERR_STAT_MATH_ERROR;
    float d = sqrt(c) * tgamma(c) / tgamma(sum.n / 2);
    v = new_real(d);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_wsd(arg_struct *arg) {
    phloat wsd;
    int inf;
    vartype *v;
    int err = get_summation();
    if (err != ERR_NONE)
        return err;
    wsd = (sum.y - 1);
    if (wsd == 0 || !flags.f.all_sigma) //origin of non-linearity is weighted SD
        return ERR_STAT_MATH_ERROR;
    wsd = (sum.x2y - (sum.xy * sum.xy / sum.y)) / wsd;
    if (wsd < 0)
        return ERR_STAT_MATH_ERROR;
    if (p_isinf(wsd))
        v = new_real(POS_HUGE_PHLOAT);
    else
        v = new_real(sqrt(wsd));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_wcsd(arg_struct *arg) {
    phloat wsd_c;//correction on y from a distribution
    int inf;
    vartype *v;
    int err = get_summation();
    if (err != ERR_NONE)
        return err;
    wsd_c = ((sum.y * sum.y) - sum.y2);
    if (wsd_c == 0) 
        return ERR_STAT_MATH_ERROR;
    wsd_c = ((sum.y * sum.y) - sum.y) / wsd_c;
    if (wsd_c < 0)
        return ERR_STAT_MATH_ERROR;
    if (p_isinf(wsd_c))
        v = new_real(POS_HUGE_PHLOAT);
    else
        v = new_real(sqrt(wsd_c));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_slope(arg_struct *arg) {
    int err = get_model_summation(get_model());
    vartype *v;
    if (err != ERR_NONE)
        return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
        return err;
    if(flags.f.q_fit && flags.f.q_sigma) {
        v = new_complex(model.qslope, model.quad);
    } else {
        v = new_real(model.slope);
    }
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_sum(arg_struct *arg) {
    int err = get_summation();
    vartype *sx, *sy;
    if (err != ERR_NONE)
        return err;
    sx = new_real(sum.x);
    if (sx == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    sy = new_real(sum.y);
    if (sy == NULL) {
        free_vartype(sx);
        return ERR_INSUFFICIENT_MEMORY;
    }
    free_vartype(reg_lastx);
    free_vartype(reg_y);
    reg_y = sy;
    reg_lastx = reg_x;
    reg_x = sx;
    if (flags.f.trace_print && flags.f.printer_enable)
        docmd_prx(NULL);
    return ERR_NONE;
}

int docmd_wmean(arg_struct *arg) {
    phloat wm;
    int inf;
    vartype *v;
    int err = get_summation();
    if (err != ERR_NONE)
        return err;
    if (sum.y == 0)
        return ERR_STAT_MATH_ERROR;
    wm = sum.xy / sum.y;
    if ((inf = p_isinf(wm)) != 0)
        wm = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    v = new_real(wm);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_yint(arg_struct *arg) {
    int err = get_model_summation(get_model());
    phloat yint;
    vartype *v;
    if (err != ERR_NONE)
        return err;
    err = slope_yint_helper();
    if (err != ERR_NONE)
        return err;
    yint = (flags.f.q_fit && flags.f.q_sigma) ? model.qyint : model.yint;
    if (model.exp_after) {
        yint = exp(yint);
        if (p_isinf(yint) != 0)
            yint = POS_HUGE_PHLOAT;
    }
    v = new_real(yint);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    recall_result(v);
    return ERR_NONE;
}

int docmd_addr(arg_struct *arg) {
    vartype *res, *sub, *add;
    int error = generic_add(reg_x, reg_y, &res);
    if (error != ERR_NONE) return error;
    error = generic_sub(res, reg_x, &sub);
    if (error != ERR_NONE) return error;
    error = generic_sub(reg_y, sub, &add);
    if (error != ERR_NONE) return error;
    free_vartype(sub);
    free_vartype(reg_lastx);
    reg_lastx = reg_x;
    free_vartype(reg_y);
    //free_vartype(reg_x);
    reg_y = res;
    unary_result(add);//print trace
    return ERR_NONE;
}

static vartype *args_s = NULL;
static vartype *var_s = NULL;

int docmd_ssto(arg_struct *arg) {
    int err;
    if(var_s != NULL) {
        err = store_var("\x06M", 2, dup_vartype(var_s), true);//local mvar
        if(err != ERR_NONE) return err;
    }
    err = store_var("\x06X", 2, dup_vartype(reg_x), true);//X
    if(err != ERR_NONE) return err;
    err = store_var("\x06Y", 2, dup_vartype(reg_y), true);//Y
    if(err != ERR_NONE) return err;
    err = store_var("\x06Z", 2, dup_vartype(reg_z), true);//Z
    if(err != ERR_NONE) return err;
    err = store_var("\x06T", 2, dup_vartype(reg_t), true);//T
    if(err != ERR_NONE) return err;
    err = store_var("\x06LX", 3, dup_vartype(reg_lastx), true);//LX
    return err;
}

int docmd_gen(arg_struct *arg) {
    int err;
    if(args_s == NULL) return ERR_LABEL_NOT_FOUND;
    arg_struct args;
    args.type = ARGTYPE_STR;
    int len;
    string_copy(args.val.text, &len,
                ((vartype_string *)args_s)->text, ((vartype_string *)args_s)->length);
    args.length = len;
    err = docmd_xeq(&args);//and indirect execute it
    if(err != ERR_NONE) return err;
    err = store_var("\x06G", 2, dup_vartype(args_s), true);//local pgmint
    if(err != ERR_NONE) return err;
    //defer
    return docmd_ssto(arg);//chain
}

int docmd_srcl(arg_struct *arg) {
    vartype *r = recall_var("\x06G", 2);
    if(r != NULL) {//outer gen
        if(args_s != NULL) free_vartype(args_s);//safety store
        args_s = dup_vartype(r);//restore outer gen
    }
    r = recall_var("\x06M", 2);
    if(r != NULL) {//outer mvar
        if(var_s != NULL) free_vartype(var_s);//safety store
        var_s = dup_vartype(r);//restore outer gen
    }
    vartype *x, *y, *z, *t, *lx;
    x = recall_var("\x06X", 2);
    y = recall_var("\x06Y", 2);
    z = recall_var("\x06Z", 2);
    t = recall_var("\x06T", 2);
    lx = recall_var("\x06LX", 3);
    if(x == NULL || y == NULL || z == NULL
            || t == NULL || lx == NULL) return ERR_NONEXISTENT;
    free_vartype(reg_x);
    free_vartype(reg_y);
    free_vartype(reg_z);
    free_vartype(reg_t);
    free_vartype(reg_lastx);
    reg_y = dup_vartype(y);
    reg_z = dup_vartype(z);
    reg_t = dup_vartype(t);
    reg_lastx = dup_vartype(lx); 
    unary_result(x);//print trace
    return ERR_NONE;
}

int docmd_atom(arg_struct *arg) {
    //Read VARMENU docs
    if(var_s != NULL) free_vartype(var_s);//safe
    var_s = new_string(reg_alpha, reg_alpha_length);
    return ERR_NONE;
}

//programatic limitation (no literal entry MVAR "xxx")
static vartype* mvar_read() {
    if(var_s == NULL || ((vartype_string *)var_s)->length == 0) return NULL;
    vartype *r = recall_var(((vartype_string *)var_s)->text,
            ((vartype_string *)var_s)->length);
    if(r == NULL) return NULL;
    return dup_vartype(r);//new instance
}

static int mvar_write(vartype *val) {
    if(var_s == NULL || ((vartype_string *)var_s)->length == 0) return ERR_NONEXISTENT;
    return store_var(((vartype_string *)var_s)->text,
            ((vartype_string *)var_s)->length, dup_vartype(val), true);//temporary local
}

int docmd_msto(arg_struct *arg) {
    return mvar_write(reg_x);
}

int docmd_mrcl(arg_struct *arg) {
    vartype *r = mvar_read();
    if(r == NULL) return ERR_NONEXISTENT;
    recall_result(r);
    return ERR_NONE;
}

int docmd_integ(arg_struct *arg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;
    if (!program_running())
        clear_all_rtns();
    string_copy(reg_alpha, &reg_alpha_length, arg->val.text, arg->length);
    return start_integ(arg->val.text, arg->length);
}

int docmd_linsigma(arg_struct *arg) {
    flags.f.all_sigma = 0;
    flags.f.q_sigma = 0;
    return ERR_NONE;
}

int docmd_not(arg_struct *arg) {
    int8 x;
    int err;
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE)
        return err;
    int8 res = ~x;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    unary_result(v);
    return ERR_NONE;
}

int docmd_or(arg_struct *arg) {
    int8 x, y; 
    int err; 
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE) 
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    int8 res = x | y;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL) 
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_pgmslv(arg_struct *arg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type == ARGTYPE_STR) {
        int prgm;
        int4 pc;
        if (!find_global_label(arg, &prgm, &pc))
            return ERR_LABEL_NOT_FOUND;
        set_solve_prgm(arg->val.text, arg->length);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_pgmint(arg_struct *arg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type == ARGTYPE_STR) {
        int prgm;
        int4 pc;
        if (!find_global_label(arg, &prgm, &pc))
            return ERR_LABEL_NOT_FOUND;
        set_integ_prgm(arg->val.text, arg->length);
        if(args_s != NULL) free_vartype(args_s);//safety store
        args_s = new_string(arg->val.text, arg->length);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int appmenu_exitcallback_3(int menuid, bool exitall) {
    if (menuid == MENU_NONE && !exitall) {
        set_menu(MENULEVEL_APP, MENU_CATALOG);
        set_cat_section(CATSECT_PGM_SOLVE);
    } else
        mode_appmenu = menuid;
    return ERR_NONE;
}

int docmd_pgmslvi(arg_struct *arg) {
    /* This command can only be invoked from a menu; we assume that
     * the menu handler only gives us valid arguments. We do check
     * the argument type, but the existence of the named label, and
     * whether it actually has MVAR instructions, we just assume.
     */
    if (arg->type == ARGTYPE_STR) {
        set_solve_prgm(arg->val.text, arg->length);
        string_copy(varmenu, &varmenu_length, arg->val.text, arg->length);
        varmenu_row = 0;
        varmenu_role = 1;
        set_menu(MENULEVEL_APP, MENU_VARMENU);
        set_appmenu_exitcallback(3);
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int appmenu_exitcallback_4(int menuid, bool exitall) {
    if (menuid == MENU_NONE && !exitall) {
        set_menu(MENULEVEL_APP, MENU_CATALOG);
        set_cat_section(CATSECT_PGM_INTEG);
    } else
        mode_appmenu = menuid;
    return ERR_NONE;
}

int appmenu_exitcallback_5(int menuid, bool exitall) {
    if (menuid == MENU_NONE && !exitall) {
        get_integ_prgm(varmenu, &varmenu_length);
        varmenu_row = 0;
        varmenu_role = 2;
        set_menu(MENULEVEL_APP, MENU_VARMENU);
        set_appmenu_exitcallback(4);
    } else
        mode_appmenu = menuid;
    return ERR_NONE;
}

int docmd_pgminti(arg_struct *arg) {
    /* This command can only be invoked from a menu; we assume that
     * the menu handler only gives us valid arguments. We do check
     * the argument type, but the existence of the named label, and
     * whether it actually has MVAR instructions, we just assume.
     */
    if (arg->type == ARGTYPE_STR) {
        set_integ_prgm(arg->val.text, arg->length);
        string_copy(varmenu, &varmenu_length, arg->val.text, arg->length);
        varmenu_row = 0;
        varmenu_role = 2;
        set_menu(MENULEVEL_APP, MENU_VARMENU);
        set_appmenu_exitcallback(4);
        clear_row(0);
        draw_string(0, 0, "Set Vars; Select \003var", 21);
        flags.f.message = 1;
        flags.f.two_line_message = 0;
        mode_varmenu = true;
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_rotxy(arg_struct *arg) {
    int x;
    uint8 y, res;
    int err; 
    vartype *v;

    // Not using get_base_param() to fetch x, because that
    // would make it impossible to specify a negative shift
    // count in unsigned mode.
    if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else if (reg_x->type != TYPE_REAL)
        return ERR_INVALID_TYPE;
    phloat px = floor(((vartype_real *) reg_x)->x);
    int wsize = effective_wsize();
    if (px >= wsize || px <= -wsize)
        return ERR_INVALID_DATA;
    x = to_int(px);

    if ((err = get_base_param(reg_y, (int8 *) &y)) != ERR_NONE)
        return err;
    if (x == 0)
        res = y;
    else {
        if (wsize < 64)
            y &= (1ULL << wsize) - 1;
        if (x > 0)
            res = (y >> x) | (y << (wsize - x));
        else {
            x = -x;
            res = (y << x) | (y >> (wsize - x));
        }
        base_range_check((int8 *) &res, true);
    }
    v = new_real(base2phloat((int8) res));
    if (v == NULL) 
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_solve(arg_struct *arg) {
    int err;
    vartype *v;
    phloat x1, x2;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;

    v = recall_var(arg->val.text, arg->length);
    if (v == 0)
        x1 = 0;
    else if (v->type == TYPE_REAL)
        x1 = ((vartype_real *) v)->x;
    else if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;

    if (reg_x->type == TYPE_REAL)
        x2 = ((vartype_real *) reg_x)->x;
    else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;

    if (!program_running())
        clear_all_rtns();
    string_copy(reg_alpha, &reg_alpha_length, arg->val.text, arg->length);
    return start_solve(arg->val.text, arg->length, x1, x2);
}

int docmd_vmsolve(arg_struct *arg) {
    vartype *v;
    phloat x1, x2;
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;

    v = recall_var(arg->val.text, arg->length);
    if (v == NULL) {
        x1 = 0;
        x2 = 1;
    } else if (v->type == TYPE_REAL) {
        x1 = ((vartype_real *) v)->x;
        if (!get_shadow(arg->val.text, arg->length, &x2))
            x2 = x1;
    } else if (v->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;

    clear_all_rtns();
    string_copy(reg_alpha, &reg_alpha_length, arg->val.text, arg->length);
    return start_solve(arg->val.text, arg->length, x1, x2);
}

int docmd_xor(arg_struct *arg) {
    int8 x, y;
    int err; 
    vartype *v;
    if ((err = get_base_param(reg_x, &x)) != ERR_NONE) 
        return err;
    if ((err = get_base_param(reg_y, &y)) != ERR_NONE)
        return err;
    int8 res = x ^ y;
    base_range_check(&res, true);
    v = new_real(base2phloat(res));
    if (v == NULL) 
        return ERR_INSUFFICIENT_MEMORY;
    binary_result(v);
    return ERR_NONE;
}

int docmd_to_dec(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        phloat oct = ((vartype_real *) reg_x)->x;
        phloat res;
        int neg = oct < 0;
        if (neg)
            oct = -oct;
        if (oct > 777777777777.0 || oct != floor(oct))
            return ERR_INVALID_DATA;
        vartype *v;
        #ifdef BCD_MATH
            phloat dec = 0, mul = 1;
            while (oct != 0) {
                int digit = to_digit(oct);
                if (digit > 7)
                    return ERR_INVALID_DATA;
                oct = floor(oct / 10);
                dec += digit * mul;
                mul *= 8;
            }
            res = neg ? -dec : dec;
        #else
            int8 ioct = to_int8(oct);
            int8 dec = 0, mul = 1;
            while (ioct != 0) {
                int digit = (int) (ioct % 10);
                if (digit > 7)
                    return ERR_INVALID_DATA;
                ioct /= 10;
                dec += digit * mul;
                mul <<= 3;
            }
            res = (double) (neg ? -dec : dec);
        #endif
        v = new_real(res);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        unary_result(v);
        return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_to_oct(arg_struct *arg) {
    if (reg_x->type == TYPE_REAL) {
        phloat dec = ((vartype_real *) reg_x)->x;
        phloat res;
        int neg = dec < 0;
        if (neg)
            dec = -dec;
        if (dec > 68719476735.0 || dec != floor(dec))
            return ERR_INVALID_DATA;
        vartype *v;
        #ifdef BCD_MATH
            phloat oct = 0, mul = 1;
            while (dec != 0) {
                int digit = to_int(fmod(dec, 8));
                dec = floor(dec / 8);
                oct += digit * mul;
                mul *= 10;
            }
            res = neg ? -oct : oct;
        #else
            int8 idec = to_int8(dec);
            int8 oct = 0, mul = 1;
            while (idec != 0) {
                int digit = (int) (idec & 7);
                idec >>= 3;
                oct += digit * mul;
                mul *= 10;
            }
            res = (double) (neg ? -oct : oct);
        #endif
        v = new_real(res);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
        unary_result(v);
        return ERR_NONE;
    } else if (reg_x->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

static void accum(phloat *sum, phloat term, int weight) {
    int inf;
    phloat s;
    if (weight == 1)
        s = *sum + term;
    else
        s = *sum - term;
    if ((inf = p_isinf(s)) != 0)
        s = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
    *sum = s;
}

static phloat sigma_helper_2(phloat *sigmaregs,
                             phloat x, phloat y, int weight) {

    accum(&sigmaregs[0], x, weight);
    accum(&sigmaregs[1], x * x, weight);
    accum(&sigmaregs[2], y, weight);
    accum(&sigmaregs[3], y * y, weight);
    accum(&sigmaregs[4], x * y, weight);
    accum(&sigmaregs[5], 1, weight);

    if (flags.f.all_sigma) {
        accum(&sigmaregs[13], x * x * y, weight);//clobber fix
        if(flags.f.q_sigma) {
            accum(&sigmaregs[14], x * x * x, weight);
            accum(&sigmaregs[15], x * x * x * x, weight);
        }
        if (x > 0) {
            phloat lnx = log(x);
            if (y > 0) {
                phloat lny = log(y);
                accum(&sigmaregs[8], lny, weight);
                accum(&sigmaregs[9], lny * lny, weight);
                accum(&sigmaregs[10], lnx * lny, weight);
                accum(&sigmaregs[11], x * lny, weight);
                if(flags.f.q_sigma) {
                    accum(&sigmaregs[20], lnx * lnx * lny, weight);
                }
            } else {
                flags.f.exp_fit_invalid = 1;
                flags.f.pwr_fit_invalid = 1;
            }
            accum(&sigmaregs[6], lnx, weight);
            accum(&sigmaregs[7], lnx * lnx, weight);
            accum(&sigmaregs[12], lnx * y, weight);
            if(flags.f.q_sigma) {
                accum(&sigmaregs[16], lnx * lnx * lnx, weight);
                accum(&sigmaregs[17], lnx * lnx * lnx * lnx, weight);
                accum(&sigmaregs[18], lnx * lnx * y, weight);
            }
        } else {
            if (y > 0) {
                phloat lny = log(y);
                accum(&sigmaregs[8], lny, weight);
                accum(&sigmaregs[9], lny * lny, weight);
                accum(&sigmaregs[11], x * lny, weight);
                if(flags.f.q_sigma) {
                    accum(&sigmaregs[19], x * x * lny, weight);
                }
            } else
                flags.f.exp_fit_invalid = 1;
            flags.f.log_fit_invalid = 1;
            flags.f.pwr_fit_invalid = 1;
        }
    } else {
        flags.f.log_fit_invalid = 1;
        flags.f.exp_fit_invalid = 1;
        flags.f.pwr_fit_invalid = 1;
    }

    return sigmaregs[5];
}

static int sigma_helper_1(int weight) {
    /* Check if summation registers are OK */
    int4 first = mode_sigma_reg;
    int4 last = first + (flags.f.all_sigma ? 14 : 6);
    int4 size, i;
    vartype *regs = recall_var("REGS", 4);
    vartype_realmatrix *r;
    phloat *sigmaregs;
    if (regs == NULL)
        return ERR_SIZE_ERROR;
    if (regs->type != TYPE_REALMATRIX)
        return ERR_INVALID_TYPE;
    r = (vartype_realmatrix *) regs;
    size = r->rows * r->columns;
    if (last > size)
        return ERR_SIZE_ERROR;
    for (i = first; i < last; i++)
        if (r->array->is_string[i])
            return ERR_ALPHA_DATA_IS_INVALID;
    sigmaregs = r->array->data + first;

    /* All summation registers present, real-valued, non-string. */
    switch (reg_x->type) {
        case TYPE_REAL: {
            if (reg_y->type == TYPE_REAL) {
                vartype_real *x = (vartype_real *) new_real(0);
                if (x == NULL)
                    return ERR_INSUFFICIENT_MEMORY;
                x->x = sigma_helper_2(sigmaregs,
                                      ((vartype_real *) reg_x)->x,
                                      ((vartype_real *) reg_y)->x,
                                      weight);
                free_vartype(reg_lastx);
                reg_lastx = reg_x;
                reg_x = (vartype *) x;
                mode_disable_stack_lift = true;
                return ERR_NONE;
            } else if (reg_y->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else
                return ERR_INVALID_TYPE;
        }
        case TYPE_REALMATRIX: {
            vartype_realmatrix *rm = (vartype_realmatrix *) reg_x;
            vartype_real *x;
            int4 i;
            if (rm->columns != 2)
                return ERR_DIMENSION_ERROR;
            for (i = 0; i < rm->rows * 2; i++)
                if (rm->array->is_string[i])
                    return ERR_ALPHA_DATA_IS_INVALID;
            x = (vartype_real *) new_real(0);
            if (x == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            for (i = 0; i < rm->rows; i++)
                x->x = sigma_helper_2(sigmaregs,
                                      rm->array->data[i * 2],
                                      rm->array->data[i * 2 + 1],
                                      weight);
            free_vartype(reg_lastx);
            reg_lastx = reg_x;
            reg_x = (vartype *) x;
            mode_disable_stack_lift = true;
            return ERR_NONE;
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
}

int docmd_sigmaadd(arg_struct *arg) {
    int err = sigma_helper_1(1);
    if (err == ERR_NONE && flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
    return err;
}

int docmd_sigmasub(arg_struct *arg) {
    int err = sigma_helper_1(-1);
    if (err == ERR_NONE && flags.f.trace_print && flags.f.printer_exists)
        docmd_prx(NULL);
    return err;
}
