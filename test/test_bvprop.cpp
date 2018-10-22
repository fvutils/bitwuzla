/*  Boolector: Satisfiability Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2018 Mathias Preiner.
 *  Copyright (C) 2018-2019 Aina Niemetz.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#include "test.h"

extern "C" {
#include "bzlabvprop.h"
}

class TestBvProp : public TestMm
{
 protected:
  static constexpr uint32_t TEST_BW         = 3;
  static constexpr uint32_t TEST_NUM_CONSTS = 27;
  static constexpr uint32_t TEST_CONST_LEN  = (TEST_BW + 1);

  void SetUp() override
  {
    TestMm::SetUp();
    /* Initialize all possible values for 3-valued constants of bit-width
     * TEST_BW. */
    char bit     = '0';
    size_t psize = TEST_NUM_CONSTS;
    for (size_t i = 0; i < TEST_BW; i++)
    {
      psize = psize / 3;
      for (size_t j = 0; j < TEST_NUM_CONSTS; j++)
      {
        d_consts[j][i] = bit;
        if ((j + 1) % psize == 0)
        {
          bit = bit == '0' ? '1' : (bit == '1' ? 'x' : '0');
        }
      }
    }
  }

  void print_domain(BzlaBvDomain *d)
  {
    char *s;
    s = bzla_bv_to_char(d_mm, d->lo);
    printf("lo: %s, ", s);
    bzla_mem_freestr(d_mm, s);
    s = bzla_bv_to_char(d_mm, d->hi);
    printf("hi: %s\n", s);
    bzla_mem_freestr(d_mm, s);
  }

  /* Create 2-valued bit-vector from 3-valued bit-vector 'bv' by initializing
   * 'x' values to 'bit'. */
  BzlaBitVector *to_bv(const char *c, char bit)
  {
    size_t len = strlen(c);
    char buf[len + 1];
    buf[len] = '\0';
    for (size_t i = 0; i < len; i++)
    {
      buf[i] = (c[i] == 'x') ? bit : c[i];
    }
    return bzla_bv_char_to_bv(d_mm, buf);
  }

  /* Create hi for domain from 3-valued bit-vector 'bv'. */
  BzlaBitVector *to_hi(const char *bv) { return to_bv(bv, '1'); }

  /* Create lo for domain from 3-valued bit-vector 'bv'. */
  BzlaBitVector *to_lo(const char *bv) { return to_bv(bv, '0'); }

  /* Create domain from 3-valued bit-vector 'bv'. */
  BzlaBvDomain *create_domain(const char *bv)
  {
    BzlaBitVector *lo = to_lo(bv);
    BzlaBitVector *hi = to_hi(bv);
    BzlaBvDomain *res = bzla_bvprop_new(d_mm, lo, hi);
    bzla_bv_free(d_mm, lo);
    bzla_bv_free(d_mm, hi);
    return res;
  }

  /* Create 3-valued bit-vector from domain 'd'. */
  char *from_domain(BzlaMemMgr *mm, BzlaBvDomain *d)
  {
    assert(bzla_bvprop_is_valid(mm, d));
    char *lo = bzla_bv_to_char(mm, d->lo);
    char *hi = bzla_bv_to_char(mm, d->hi);

    size_t len = strlen(lo);
    for (size_t i = 0; i < len; i++)
    {
      if (lo[i] != hi[i])
      {
        /* lo[i] == '1' && hi[i] == '0' would be an invalid domain. */
        assert(lo[i] == '0');
        assert(hi[i] == '1');
        lo[i] = 'x';
      }
    }
    bzla_mem_freestr(mm, hi);
    return lo;
  }

  bool check_const_bits(BzlaBvDomain *d, const char *expected)
  {
    assert(bzla_bvprop_is_valid(d_mm, d));
    size_t len = strlen(expected);
    uint32_t bit_lo, bit_hi;
    bool res = true;

    for (size_t i = 0; i < len && res; i++)
    {
      bit_lo = bzla_bv_get_bit(d->lo, len - 1 - i);
      bit_hi = bzla_bv_get_bit(d->hi, len - 1 - i);
      if (expected[i] == 'x')
      {
        res &= bit_lo != bit_hi;
      }
      else
      {
        res &= bit_lo == bit_hi;
      }
    }
    return res;
  }

  bool is_false_eq(const char *a, const char *b)
  {
    assert(strlen(a) == strlen(b));
    size_t len = strlen(a);
    for (size_t i = 0; i < len; i++)
    {
      if (a[i] == 'x' || b[i] == 'x')
      {
        continue;
      }
      if (a[i] != b[i])
      {
        return true;
      }
    }
    return false;
  }

  bool is_true_eq(const char *a, const char *b)
  {
    assert(strlen(a) == strlen(b));
    size_t len = strlen(a);
    for (size_t i = 0; i < len; i++)
    {
      if (a[i] == 'x' && b[i] == 'x')
      {
        return false;
      }
      if (a[i] != 'x' && b[i] != 'x')
      {
        if (a[i] != b[i])
        {
          return false;
        }
      }
    }
    return true;
  }

  bool check_not(BzlaBvDomain *d_x, BzlaBvDomain *d_z)
  {
    bool res    = true;
    char *str_x = from_domain(d_mm, d_x);
    char *str_z = from_domain(d_mm, d_z);
    assert(strlen(str_x) == strlen(str_z));

    size_t len = strlen(str_x);
    for (size_t i = 0; i < len; i++)
    {
      assert(str_x[i] != 'x' || str_z[i] == 'x');
      assert(str_x[i] != '0' || str_z[i] == '1');
      assert(str_x[i] != '1' || str_z[i] == '0');
      assert(str_z[i] != '0' || str_x[i] == '1');
      assert(str_z[i] != '1' || str_x[i] == '0');
    }
    bzla_mem_freestr(d_mm, str_x);
    bzla_mem_freestr(d_mm, str_z);
    return res;
  }

  void check_sll_const(BzlaBvDomain *d_x, BzlaBvDomain *d_z, uint32_t n)
  {
    char *str_x = from_domain(d_mm, d_x);
    char *str_z = from_domain(d_mm, d_z);
    assert(strlen(str_x) == strlen(str_z));

    size_t len = strlen(str_x);
    for (size_t i = 0; i < len; i++)
    {
      assert(i >= n || str_z[len - 1 - i] == '0');
      assert(i < n || str_z[len - 1 - i] == str_x[len - 1 - i + n]);
    }
    bzla_mem_freestr(d_mm, str_x);
    bzla_mem_freestr(d_mm, str_z);
  }

  void check_srl_const(BzlaBvDomain *d_x, BzlaBvDomain *d_z, uint32_t n)
  {
    char *str_x = from_domain(d_mm, d_x);
    char *str_z = from_domain(d_mm, d_z);
    assert(strlen(str_x) == strlen(str_z));

    size_t len = strlen(str_x);
    for (size_t i = 0; i < len; i++)
    {
      assert(i >= n || str_z[i] == '0');
      assert(i < n || str_z[i] == str_x[i - n]);
    }
    bzla_mem_freestr(d_mm, str_x);
    bzla_mem_freestr(d_mm, str_z);
  }
  void test_shift_const(bool is_srl)
  {
    size_t i, j;
    uint32_t n;
    BzlaBitVector *bv_n;
    BzlaBvDomain *d_x, *d_z, *res_x, *res_z;

    for (j = 0; j < 2; j++)
    {
      if (j)
        d_z = bzla_bvprop_new_init(d_mm, TEST_BW);
      else
        d_x = bzla_bvprop_new_init(d_mm, TEST_BW);

      for (i = 0; i < TEST_NUM_CONSTS; i++)
      {
        if (j)
          d_x = create_domain(d_consts[i]);
        else
          d_z = create_domain(d_consts[i]);

        for (n = 0; n < TEST_BW + 1; n++)
        {
          bv_n = bzla_bv_uint64_to_bv(d_mm, n, TEST_BW);
          if (is_srl)
            bzla_bvprop_srl_const(d_mm, d_x, d_z, bv_n, &res_x, &res_z);
          else
            bzla_bvprop_sll_const(d_mm, d_x, d_z, bv_n, &res_x, &res_z);

          assert(bzla_bvprop_is_valid(d_mm, res_x));
          assert(bzla_bvprop_is_valid(d_mm, res_z));
          assert(j == 0
                 || bzla_bvprop_is_fixed(d_mm, d_x)
                        == bzla_bvprop_is_fixed(d_mm, res_x));
          if (is_srl)
            check_srl_const(res_x, res_z, n);
          else
            check_sll_const(res_x, res_z, n);

          bzla_bvprop_free(d_mm, res_x);
          bzla_bvprop_free(d_mm, res_z);
          bzla_bv_free(d_mm, bv_n);
        }
        if (j)
          bzla_bvprop_free(d_mm, d_x);
        else
          bzla_bvprop_free(d_mm, d_z);
      }

      if (j)
        bzla_bvprop_free(d_mm, d_z);
      else
        bzla_bvprop_free(d_mm, d_x);
    }
  }

  char d_consts[TEST_NUM_CONSTS][TEST_CONST_LEN] = {{0}};
};

TEST_F(TestBvProp, valid_domain)
{
  BzlaBitVector *lo, *hi;
  BzlaBvDomain *d;

  /* check valid */
  lo = bzla_bv_char_to_bv(d_mm, "0101011");
  hi = bzla_bv_char_to_bv(d_mm, "1101011");
  d  = bzla_bvprop_new(d_mm, lo, hi);

  assert(bzla_bvprop_is_valid(d_mm, d));
  bzla_bv_free(d_mm, lo);
  bzla_bv_free(d_mm, hi);
  bzla_bvprop_free(d_mm, d);

  /* check invalid */
  lo = bzla_bv_char_to_bv(d_mm, "1101011");
  hi = bzla_bv_char_to_bv(d_mm, "0101011");
  d  = bzla_bvprop_new(d_mm, lo, hi);

  assert(!bzla_bvprop_is_valid(d_mm, d));
  bzla_bv_free(d_mm, lo);
  bzla_bv_free(d_mm, hi);
  bzla_bvprop_free(d_mm, d);
}

TEST_F(TestBvProp, fixed_domain)
{
  BzlaBitVector *lo, *hi;
  BzlaBvDomain *d;

  /* check fixed */
  lo = bzla_bv_char_to_bv(d_mm, "0001111");
  hi = bzla_bv_char_to_bv(d_mm, "0001111");
  d  = bzla_bvprop_new(d_mm, lo, hi);

  assert(bzla_bvprop_is_fixed(d_mm, d));
  bzla_bv_free(d_mm, lo);
  bzla_bv_free(d_mm, hi);
  bzla_bvprop_free(d_mm, d);

  /* check not fixed */
  lo = bzla_bv_char_to_bv(d_mm, "0001111");
  hi = bzla_bv_char_to_bv(d_mm, "0001011");
  d  = bzla_bvprop_new(d_mm, lo, hi);

  assert(!bzla_bvprop_is_fixed(d_mm, d));
  bzla_bv_free(d_mm, lo);
  bzla_bv_free(d_mm, hi);
  bzla_bvprop_free(d_mm, d);
}

TEST_F(TestBvProp, init_domain)
{
  BzlaBvDomain *d = bzla_bvprop_new_init(d_mm, 32);
  assert(bzla_bvprop_is_valid(d_mm, d));
  assert(!bzla_bvprop_is_fixed(d_mm, d));
  bzla_bvprop_free(d_mm, d);
}

TEST_F(TestBvProp, eq)
{
  char *str_z;
  BzlaBvDomain *d_x, *d_y, *res_xy, *res_z;

  for (size_t i = 0; i < TEST_NUM_CONSTS; i++)
  {
    d_x = create_domain(d_consts[i]);
    for (size_t j = 0; j < TEST_NUM_CONSTS; j++)
    {
      d_y = create_domain(d_consts[j]);
      bzla_bvprop_eq(d_mm, d_x, d_y, &res_xy, &res_z);
      if (bzla_bvprop_is_fixed(d_mm, res_z))
      {
        str_z = from_domain(d_mm, res_z);
        assert(strlen(str_z) == 1);
        assert(str_z[0] == '0' || str_z[0] == '1');
        if (str_z[0] == '0')
        {
          assert(!bzla_bvprop_is_valid(d_mm, res_xy));
          assert(is_false_eq(d_consts[i], d_consts[j]));
        }
        else
        {
          assert(str_z[0] == '1');
          assert(bzla_bvprop_is_valid(d_mm, res_xy));
          assert(bzla_bvprop_is_fixed(d_mm, res_xy));
          assert(is_true_eq(d_consts[i], d_consts[j]));
        }
        bzla_mem_freestr(d_mm, str_z);
      }
      else
      {
        assert(bzla_bvprop_is_valid(d_mm, res_xy));
        assert(!is_false_eq(d_consts[i], d_consts[j]));
        assert(!is_true_eq(d_consts[i], d_consts[j]));
      }
      bzla_bvprop_free(d_mm, d_y);
      bzla_bvprop_free(d_mm, res_xy);
      bzla_bvprop_free(d_mm, res_z);
    }
    bzla_bvprop_free(d_mm, d_x);
  }
}

TEST_F(TestBvProp, not )
{
  BzlaBvDomain *d_x, *d_z, *res_x, *res_z;

  for (size_t i = 0; i < TEST_NUM_CONSTS; i++)
  {
    d_x = create_domain(d_consts[i]);

    for (size_t j = 0; j < TEST_NUM_CONSTS; j++)
    {
      d_z = create_domain(d_consts[j]);
      bzla_bvprop_not(d_mm, d_x, d_z, &res_x, &res_z);

      if (bzla_bvprop_is_valid(d_mm, res_z))
      {
        assert(bzla_bvprop_is_valid(d_mm, res_x));
        assert(!bzla_bvprop_is_fixed(d_mm, d_z)
               || (bzla_bvprop_is_fixed(d_mm, res_x)
                   && bzla_bvprop_is_fixed(d_mm, res_z)));
        check_not(res_x, res_z);
      }
      else
      {
        bool valid = true;
        for (size_t k = 0; k < TEST_BW && valid; k++)
        {
          if (d_consts[i][k] != 'x' && d_consts[i][k] == d_consts[j][k])
          {
            valid = false;
          }
        }
        assert(!valid);
      }
      bzla_bvprop_free(d_mm, d_z);
      bzla_bvprop_free(d_mm, res_x);
      bzla_bvprop_free(d_mm, res_z);
    }
    bzla_bvprop_free(d_mm, d_x);
  }
}

TEST_F(TestBvProp, sll) { test_shift_const(false); }

TEST_F(TestBvProp, srl) { test_shift_const(true); }

TEST_F(TestBvProp, and)
{
  BzlaBvDomain *d_x, *d_y, *d_z;
  BzlaBvDomain *res_x, *res_y, *res_z;

  for (size_t i = 0; i < TEST_NUM_CONSTS; i++)
  {
    d_z = create_domain(d_consts[i]);
    for (size_t j = 0; j < TEST_NUM_CONSTS; j++)
    {
      d_x = create_domain(d_consts[j]);
      for (size_t k = 0; k < TEST_NUM_CONSTS; k++)
      {
        d_y = create_domain(d_consts[k]);

        bzla_bvprop_and(d_mm, d_x, d_y, d_z, &res_x, &res_y, &res_z);

        if (bzla_bvprop_is_valid(d_mm, res_z))
        {
          assert(bzla_bvprop_is_valid(d_mm, res_x));
          assert(bzla_bvprop_is_valid(d_mm, res_y));

          for (size_t l = 0; l < TEST_BW; l++)
          {
            assert(d_consts[i][l] != '1'
                   || (d_consts[j][l] != '0' && d_consts[k][l] != '0'));
            assert(d_consts[i][l] != '0'
                   || (d_consts[j][l] != '1' || d_consts[k][l] != '1'));
          }
        }
        else
        {
          bool valid = true;
          for (size_t l = 0; l < TEST_BW && valid; l++)
          {
            if ((d_consts[i][l] == '0' && d_consts[j][l] != '0'
                 && d_consts[k][l] != '0')
                || (d_consts[i][l] == '1'
                    && (d_consts[j][l] == '0' || d_consts[k][l] == '0')))
            {
              valid = false;
            }
          }
          assert(!valid);
        }
        bzla_bvprop_free(d_mm, d_y);
        bzla_bvprop_free(d_mm, res_x);
        bzla_bvprop_free(d_mm, res_y);
        bzla_bvprop_free(d_mm, res_z);
      }
      bzla_bvprop_free(d_mm, d_x);
    }
    bzla_bvprop_free(d_mm, d_z);
  }
}

TEST_F(TestBvProp, slice)
{
  char buf[TEST_BW + 1];
  BzlaBvDomain *d_x, *d_z, *res_x, *res_z;

  for (size_t i = 0; i < TEST_NUM_CONSTS; i++)
  {
    d_x = create_domain(d_consts[i]);
    for (size_t j = 0; j < TEST_NUM_CONSTS; j++)
    {
      for (uint32_t lower = 0; lower < TEST_BW; lower++)
      {
        for (uint32_t upper = lower; upper < TEST_BW; upper++)
        {
          memset(buf, 0, sizeof(buf));
          memcpy(buf, &d_consts[j][TEST_BW - 1 - upper], upper - lower + 1);
          assert(strlen(buf) > 0);
          assert(strlen(buf) == upper - lower + 1);

          d_z = create_domain(buf);
          bzla_bvprop_slice(d_mm, d_x, d_z, upper, lower, &res_x, &res_z);
          if (bzla_bvprop_is_valid(d_mm, res_z))
          {
            assert(bzla_bvprop_is_valid(d_mm, res_x));
            char *str_res_x = from_domain(d_mm, res_x);
            char *str_res_z = from_domain(d_mm, res_z);
            for (size_t k = 0; k < upper - lower + 1; k++)
            {
              assert(str_res_z[k] == str_res_x[TEST_BW - 1 - upper + k]);
            }
            bzla_mem_freestr(d_mm, str_res_x);
            bzla_mem_freestr(d_mm, str_res_z);
          }
          else
          {
            assert(!bzla_bvprop_is_valid(d_mm, res_x));
            bool valid = true;
            for (size_t k = 0; k < TEST_BW; k++)
            {
              if (buf[k] != d_consts[i][TEST_BW - 1 - upper + k])
              {
                valid = false;
                break;
              }
            }
            assert(!valid);
          }
          bzla_bvprop_free(d_mm, d_z);
          bzla_bvprop_free(d_mm, res_x);
          bzla_bvprop_free(d_mm, res_z);
        }
      }
    }
    bzla_bvprop_free(d_mm, d_x);
  }
}