/*
 * tag.c - Tag.
 *
 * author       : Jeroen van der Heijden
 * email        : jeroen@transceptor.technology
 * copyright    : 2017, Transceptor Technology
 *
 * changes
 *  - initial version, 16-06-2017
 *
 */
#define _GNU_SOURCE
#include <assert.h>
#include <logger/logger.h>
#include <siri/db/tag.h>
#include <stdlib.h>
#include <siri/db/series.h>

#define TAGFN_NUMBERS 9

/*
 * Returns tag when successful or NULL in case of an error.
 */
static siridb_tag_t * TAG_new(uint64_t id, const char * tags_path)
{
	siridb_tag_t * tag = (siridb_tag_t *) malloc(sizeof(siridb_tag_t));
	if (tag != NULL)
	{
		tag->ref = 1;
		tag->flags = 0;
		tag->n = 0;
		tag->id = id;
		tag->name = NULL;
		;
		tag->series = imap_new();

		if (asprintf(
				&tag->fn,
				"%s%0*" PRIu64 ".tag",
				tags_path,
				TAGFN_NUMBERS,
				id) < 0 || tag->series == NULL)
		{
			siridb__tag_free(tag);
			tag = NULL;
		}
	}
	return tag;
}

/*
 * Returns tag when successful or NULL in case of an error.
 */
siridb_tag_t * siridb_tag_load(siridb_t * siridb, const char * fn)
{
	siridb_tag_t * tag = TAG_new(siridb->tags->path, (uint64_t) atoll(fn));
	if (tag != NULL)
	{
		qp_unpacker_t * unpacker = qp_unpacker_ff(tag->fn);
		if (unpacker == NULL)
		{
			log_critical("cannot open tag file for reading: %s", tag->fn);
			siridb__tag_free(tag);
			tag = NULL;
		}
		else
		{
			qp_obj_t qp_tn;

			if (!qp_is_array(qp_next(unpacker, NULL)) ||
				qp_next(unpacker, &qp_tn) == QP_RAW ||
				(tag->name = strndup(qp_tn->via->raw, qp_tn->len)) == NULL)
			{
				/* or a memory allocation error, but the same result */
				log_critical(
						"expected an array with a tag name in file: %s",
						tag->fn);
				siridb__tag_free(tag);
				tag = NULL;
			}
			else
			{
				qp_obj_t qp_series_id;
				uint64_t series_id;
				siridb_series_t * series;

				while (qp_next(unpacker, &qp_series_id) == QP_INT64)
				{
					series_id = (uint64_t) qp_series_id->via->int64;
					series = imap_get(siridb->series_map, series_id);

					if (series == NULL)
					{
						siridb_tags_require_save(siridb->tags, tag);

						log_error(
								"cannot find series id %" PRId64
								" which was tagged with '%s'",
								qp_series_id->via->int64,
								tag->name);
					}
					else if (imap_add(tag->series, series_id, series) == 0)
					{
						siridb_series_incref(series);
					}
					else
					{
						log_critical(
								"cannot add series '%s' to tag '%s'",
								series->name,
								tag->name);
					}
				}
			}
			qp_unpacker_ff_free(unpacker);
		}
	}
	return tag;
}

int siridb_tag_save(siridb_tag_t * tag, uv_mutex_t * lock)
{
	qp_fpacker_t * fpacker;

	fpacker = qp_open(tag->fn, "w");
	if (fpacker == NULL)
	{
		return -1;
	}

	if (/* open a new array */
		qp_fadd_type(fpacker, QP_ARRAY_OPEN) ||

		/* write the tag name */
		qp_fadd_string(fpacker, tag->name))
	{
		qp_close(fpacker);
		return -1;
	}

	uv_mutex_lock(lock);

	slist_t * series_list = imap_slist(tag->series);

	if (series_list != NULL)
	{
		siridb_series_t * series;
		for (size_t i = 0; i < series_list->len; i++)
		{
			series = (siridb_series_t *) series_list->data[i];
			qp_fadd_int64(fpacker, (int64_t) series->id);
		}
	}

	uv_mutex_unlock(lock);

	if (qp_close(fpacker) || series_list == NULL)
	{
		return -1;
	}

	return 0;
}

/*
 * Can be used as a callback, in other cases go for the macro.
 */
void siridb__tag_decref(siridb_tag_t * tag)
{
    if (!--tag->ref)
    {
        siridb__tag_free(tag);
    }
}

/*
 * NEVER call  this function but rather call siridb_tag_decref instead.
 *
 * Destroy a tag object. Parsing NULL is not allowed.
 */
void siridb__tag_free(siridb_tag_t * tag)
{
#ifdef DEBUG
    log_debug("Free tag: '%s'", tag->name);
#endif
    free(tag->name);
    free(tag->fn);
    if (tag->series != NULL)
    {
        imap_free(tag->series, (imap_free_cb) siridb__series_decref);
    }

    free(tag);
}

/*
 * Returns 1 (true) if the file name is valid and 0 (false) if not
 */
int siridb_tag_is_valid_fn(const char * fn)
{
    int i = 0;
    while (*fn && isdigit(*fn))
    {
        fn++;
        i++;
    }
    return (i == TAGFN_NUMBERS) ? (strcmp(fn, ".tag") == 0) : 0;
}


