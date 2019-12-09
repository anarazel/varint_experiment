#include "varint.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static void
print_conv(int64_t val, uint8_t *buf, int buf_len, int64_t decoded)
{
	uint64_t uval;
	uint64_t sval_be __attribute__((unused));
	int neg;

	if (val < 0)
	{
		uval = -(uint64_t) val;
		neg = true;
	}
	else
	{
		uval = (uint64_t) val;
		neg = false;
	}

	sval_be = pg_hton64(val);

	fprintf(stdout,
			"signed: %ld, unsigned: %lu, sign bit: %d\n"
			"  input bytes:\t %.2x %.2x  %.2x %.2x  %.2x %.2x  %.2x %.2x\n"
			"%d output bytes:\t ",
			val, uval,
			neg,
			(uint8_t) *((char *) &sval_be + 0),
			(uint8_t) *((char *) &sval_be + 1),
			(uint8_t) *((char *) &sval_be + 2),
			(uint8_t) *((char *) &sval_be + 3),
			(uint8_t) *((char *) &sval_be + 4),
			(uint8_t) *((char *) &sval_be + 5),
			(uint8_t) *((char *) &sval_be + 6),
			(uint8_t) *((char *) &sval_be + 7),
			buf_len);

	for (int i = 0; i < buf_len; i++)
	{
		if (i != 0)
		{
			if ((i % 2) == 0)
				fprintf(stdout, "  ");
			else
				fprintf(stdout, " ");
		}
		fprintf(stdout, "%.2x", buf[i]);
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "decoded:\t%ld\n\n", decoded);
}

static void
print_uconv(uint64_t uval, uint8_t *buf, int buf_len, uint64_t decoded)
{
	uint64_t uval_be __attribute__((unused));

	uval_be = __builtin_bswap64(uval);

	fprintf(stdout,
			"unsigned:\t%lu\n"
			"  input bytes:\t %.2x %.2x  %.2x %.2x  %.2x %.2x  %.2x %.2x\n"
			"%d output bytes:\t ",
			uval,
			(uint8_t) *((char *) &uval_be + 0),
			(uint8_t) *((char *) &uval_be + 1),
			(uint8_t) *((char *) &uval_be + 2),
			(uint8_t) *((char *) &uval_be + 3),
			(uint8_t) *((char *) &uval_be + 4),
			(uint8_t) *((char *) &uval_be + 5),
			(uint8_t) *((char *) &uval_be + 6),
			(uint8_t) *((char *) &uval_be + 7),
			buf_len);

	for (int i = 0; i < buf_len; i++)
	{
		if (i != 0)
		{
			if ((i % 2) == 0)
				fprintf(stdout, "  ");
			else
				fprintf(stdout, " ");
		}
		fprintf(stdout, "%.2x", buf[i]);
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "decoded:\t%lu\n\n", decoded);
}

static const int64_t int64_t_test_vectors[] = {
	0,
	1,
	-1,
	2,
	-2,
	3,
	-3,
	10,
	-10,
	100,
	-100,
	127,
	-127,
	257,
	-257,
	512,
	-512,
	INT_MAX,
	INT_MIN,
	(1L<<40) + 17,
	- ((1L<<40) + 17),
	(1L<<48) + 12,
	- ((1L<<48) + 12),
	(INT64_MAX - 1),
	-(INT64_MAX - 1),
	-(INT64_MIN + 1),
	INT64_MIN + 1,
	INT64_MAX,
	INT64_MIN
};

static const uint64_t uint64_t_test_vectors[] = {
	0,
	1,
	2,
	3,
	10,
	100,
	126,
	127,
	128,
	257,
	512,
	INT_MAX,
	UINT_MAX,
	(1L<<40) + 17,
	(1L<<48) + 12,
	PG_VARINT_UINT64_MAX_8BYTE_VAL - 1,
	PG_VARINT_UINT64_MAX_8BYTE_VAL,
	PG_VARINT_UINT64_MAX_8BYTE_VAL + 1,
	INT64_MAX - 1,
	INT64_MAX,
	UINT64_MAX
};

int
main(int argc, char **argv)
{
	uint8_t buf[32];
	int opt;
	bool input_signedness = true;
	bool bench = false;

	while ((opt = getopt(argc, argv, "bsu")) != -1)
	{
		switch (opt)
		{
			case 'b':
				printf("testbench\n");
				bench = true;
				break;
			case 's':
				printf("processing signed\n");
				input_signedness = true;
				break;
			case 'u':
				printf("processing unsigned\n");
				input_signedness = false;
				break;
			case '?':
			default:
				fprintf(stderr, "wrong parameters, usage: '[-s] [-u] numbers*\n");
				exit(EXIT_FAILURE);
		}
	}

	if (!bench)
	{
		const int64_t *int64_t_input = NULL;
		const uint64_t *uint64_t_input = NULL;
		int input_len = 0;

		if (optind == argc)
		{
			if (input_signedness)
			{
				int64_t_input = int64_t_test_vectors;
				input_len = sizeof(int64_t_test_vectors) / sizeof(int64_t_test_vectors[0]);
			}
			else
			{
				uint64_t_input = uint64_t_test_vectors;
				input_len = sizeof(uint64_t_test_vectors) / sizeof(uint64_t_test_vectors[0]);
			}
		}
		else
		{
			int len = argc - optind;
			void *tmp = malloc(sizeof(int64_t) * (argc - 1));

			if (!tmp)
			{
				fprintf(stderr, "out of memory");
				exit(EXIT_FAILURE);
			}

			if (input_signedness)
			{
				int64_t *stmp = tmp;

				for (int i = 0; i < len; i++)
				{
					stmp[i] = strtoll(argv[optind + i], NULL, 0);
				}

				int64_t_input = stmp;
			}
			else
			{
				uint64_t *utmp = tmp;

				for (int i = 0; i < len; i++)
				{
					utmp[i] = strtoull(argv[optind + i], NULL, 0);
				}

				uint64_t_input = utmp;
			}

			input_len = len;
		}

		for (int i = 0; i < input_len; i++)
		{
			int outlen;
			int inlen;

			if (input_signedness)
			{
				uint64_t decoded_s64 = 0;

				outlen = pg_varint_encode_int64(int64_t_input[i], buf);
				decoded_s64 = pg_varint_decode_int64(buf, &inlen);

				print_conv(int64_t_input[i], buf, outlen, decoded_s64);
			}
			else
			{
				uint64_t decoded_u64;

				outlen = pg_varint_encode_uint64(uint64_t_input[i], buf);
				decoded_u64 = pg_varint_decode_uint64(buf, &inlen);

				print_uconv(uint64_t_input[i], buf, inlen, decoded_u64);
			}
		}
	}
	else
	{
		int outlen;
		int inlen;


		if (input_signedness)
		{
			for (int64_t i = (int64_t)INT32_MIN - 1; i < ((int64_t)INT32_MAX + 1); i++)
			{
				int64_t decoded_s64;

				outlen = pg_varint_encode_int64(i, buf);
				decoded_s64 = pg_varint_decode_int64(buf, &inlen);

				if (i != decoded_s64 || outlen != inlen)
				{
					fprintf(stderr, "match fail: %ld vs %ld\n", i, decoded_s64);
					exit(EXIT_FAILURE);
				}
			}
		}
		else
		{
			int outlen;

			for (uint64_t i = 0; i < ((uint64_t)UINT32_MAX + 1); i++)
			{
				uint64_t decoded_u64;

				outlen = pg_varint_encode_uint64(i, buf);
				decoded_u64 = pg_varint_decode_uint64(buf, &inlen);

				if (i != decoded_u64 || outlen != inlen)
				{
					fprintf(stderr, "match fail: %lu vs %lu\n", i, decoded_u64);
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}
