// see header for stuff

// based on Ortham/libespm

// has some bugs
// will probably not load

#include "common.h"

#include "esp.h"

#include <zlib.h>
#include <sys/stat.h>

int only_read_editorid = 1;
int skip_field_data = 1;

#if 0
#define spamf(...) if (Buf) printf(__VA_ARGS__);
#else
#define spamf 0;
#endif

static espp plugins[PLUGINS] = { NULL };

// horrible c vector
inline void narray(revised_array **, unsigned int);
inline void grow(revised_array *);
inline void insert(revised_array *, void *);

static int disk_read(espp, void **, size_t);
static int disk_seek(espp, long int);
static long int disk_tell(espp);

#define Buf esp->buf
#define Pos esp->pos

static void read(espp esp, void **data, size_t size) {
	if (Buf)
	{
		printf("read from buf\n");
		*data = (Buf + Pos);
		Pos += size;
	}
	else
		disk_read(esp, data, size);
}

static void seek(espp esp, long int offset) {
	if (Buf)
		Pos = offset;
	else
		disk_seek(esp, offset);
}

grupp read_grp(espp);
rcdp read_rcd(espp);
rcdbp read_rcdb(espp, rcdp);

char *uncompress_record(espp, rcdp);
void loop_rcd(espp, rcdp);
void loop_grup(espp, grupp);

unsigned int hedr_num_records(espp esp)
{
	rcdbp hedr = (*(subrecord ***)esp->header->rcdbs)[0];
	float version = *(float *)(hedr->data);
	unsigned int num_records = *(unsigned int *)(hedr->data + 4);
	// printf("hedr tes4 version %f\n", version);
	return num_records;
}

api espp plugin_load(const char *path, int whole)
{
	espp esp = calloc(1, sizeof(struct esp));
	file_name(esp->filename, path, '/');
	esp->stream = fopen(path, "rb");
	assertm(
		esp->stream, path);
	fseek(esp->stream, 0L, SEEK_END);
	esp->filesize = ftell(esp->stream);
	rewind(esp->stream);
	if (whole) {
		disk_read(esp, &Buf, esp->filesize);
		seek(esp, 0);
		printf("read whole plugin into buf\n");
	}
	spamf("goign to read the esp header\n");
	esp->header = read_rcd(esp);
	esp_check_rcd(esp->header);
	narray(&esp->grups, hedr_num_records(esp));
	narray(&esp->records, 128);
	narray(&esp->large, 10);
	int num = 0;
	while(Pos < esp->filesize)
	{
	grupp grp = read_grp(esp);
	// if you want everything ready to use, uncomment this
	//loop_grup(esp, grp);
	insert(esp->grups, grp);
	}
	return esp;
}

void skip_rcdb(espp esp, rcdp rcd, size_t);
void read_rcd_buf_or_disk(espp esp, rcdp rcd, void **, size_t);

void read_rcdb_data(espp esp, rcdp rcd, rcdbp rcdb);

static void *grp_temp[40000]; // keep this high
static rcdbp rcdb_temp[40000]; // keep this high

struct form_id build_form_id(unsigned int);

grupp read_grp(espp esp)
{
	spamf("read_grp\n");
	grupp grp = calloc(1, sizeof(struct grup));
	grp->g = 'g';
	grp->id = esp->ids.grups++;
	grp->esp = esp;
	read(esp, &grp->hed, sizeof(struct grup_header));
	grp->offset = Pos;
	grp->size = grp->hed->size - sizeof(struct grup_header);
	// loop_grup(esp, grp, 1);
	seek(esp, grp->offset + grp->size);
	return grp;
}

rcdp read_rcd(espp esp)
{
	spamf("read rcd\n");

	rcdp rcd = calloc(1, sizeof(struct record));

	rcd->r = 'r';
	rcd->id = esp->ids.records++;
	rcd->esp = esp;
	read(esp, &rcd->hed, sizeof(struct record_header));
	rcd->offset = Pos;
	rcd->size = rcd->hed->size;
	rcd->partial = 1;

	rcd->form_id = build_form_id(rcd->hed->formId);

	insert(esp->records, rcd);

	spamf("printing hed sgn:\n");
	spamf("%.4s %u\n", (char *)&rcd->hed->sgn, rcd->hed->size);

	if (rcd->hed->flags & 0x00040000)
	{
		read(esp, &rcd->buf, rcd->size);
		uncompress_record(esp, rcd);
		loop_rcd(esp, rcd);
	}
	else
		loop_rcd(esp, rcd);

	seek(esp, rcd->offset + rcd->hed->size);
	return rcd;
}

rcdbp read_rcdb(espp esp, rcdp rcd)
{
	spamf("read rcdb\n");
	rcdbp rcdb = calloc(1, sizeof(struct subrecord));
	rcdb->s = 's';
	rcdb->id = esp->ids.subrecords++;
	read_rcd_buf_or_disk(esp, rcd, &rcdb->hed, sizeof(struct subrecord_header));
	rcdb->offset = rcd->buf ? rcd->pos : Pos;
	//read_rcdb_data(esp, rcd, rcdb);
	skip_rcdb(esp, rcd, rcdb->hed->size);
	return rcdb;
}

void read_rcdb_data(espp esp, rcdp rcd, rcdbp rcdb)
{
	if (rcdb->data)
		return;
	if (rcd->buf)
		rcd->pos = rcdb->offset;
	else
		seek(esp, rcdb->offset);
	rcdb->data = malloc(rcdb->hed->size);
	read_rcd_buf_or_disk(esp, rcd, &rcdb->data, rcdb->hed->size);
}

unsigned int peek_type(espp esp)
{
	unsigned int *sgn;
	read(esp, &sgn, sizeof(unsigned int));
	seek(esp, Pos - 4);
	return *sgn;
}

void loop_grup(espp esp, grupp grp)
{
	seek(grp->esp, grp->offset);
	
	unsigned start = Pos;
	unsigned int num = 0;
	while (Pos - start < grp->size)
	{
		if (peek_type(esp) == *(unsigned int *)"GRUP")
		{
			grupp grp2 = read_grp(esp);
			grp_temp[num++] = grp2;
		}
		else
		{
			rcdp rcd = read_rcd(esp);
			grp_temp[num++] = rcd;
		}
		assertm(num < 40000, "overflow");
	}
	narray(&grp->mixed, num);
	for (unsigned int i = 0; i < num; i++)
		insert(grp->mixed, grp_temp[i]);
	assertc(Pos == grp->offset + grp->size);
	grp->looped = 1;
}

void loop_rcd(espp esp, rcdp rcd)
{
	assertc(rcd->buf || Pos == rcd->offset);

	spamf("process record\n");
	if (rcd->buf)
		rcd->pos = 0;
	else
		rcd->pos = rcd->offset;
	unsigned begin = rcd->pos;
	unsigned int num = 0;
	//static rcdbp temp[15000]; // should this be static? where will it reside? in stack? in address space?
	while(rcd->pos - begin < rcd->size)
	{
		assertm(num < 40000, "overflow");
		rcdbp rcdb = read_rcdb(esp, rcd);
		//if (num == 0)
		//read_rcdb_data(esp, rcd, rcdb);

		if (rcdb->hed->sgn == *(unsigned int *)"XXXX")
		{
			// printf("oef");
			read_rcdb_data(esp, rcd, rcdb);
			rcdbp discard = read_rcdb(esp, rcd);
			if (rcd->buf)
				printf("\ncompressed oversized sub!!\n\n");
			skip_rcdb(esp, rcd, *(unsigned int *)rcdb->data);
			continue;
		}
		rcdb_temp[num++] = rcdb;
	}
	narray(&rcd->rcdbs, num);
	for (unsigned int i = 0; i < num; i++)
		insert(rcd->rcdbs, rcdb_temp[i]);
}

api void esp_check_grup(grupp grp)
{
	if (grp==NULL)
		return;
	if (!grp->looped)
		loop_grup(grp->esp, grp);
}

api void esp_check_rcd(rcdp rcd)
{
	if (rcd->partial)
	{
		//printf("checked partial rcd\n");
		for (unsigned int i = 0; i < rcd->rcdbs->size; i++)
		{
			rcdbp rcdb = rcd->rcdbs->elements[i];
			if (rcdb->data)
				continue;
			if (rcd->buf)
				rcd->pos = rcdb->offset;
			else
				seek(rcd->esp, rcdb->offset);
			read_rcdb_data(rcd->esp, rcd, rcdb);
		}
		rcd->partial = 0;
	}
}

void read_rcd_buf_or_disk(espp esp, rcdp rcd, void **dest, size_t size)
{
	if (rcd->buf)
		*dest = rcd->buf + rcd->pos;
	else
		read(esp, dest, size);
	rcd->pos += size;
}

void skip_rcdb(espp esp, rcdp rcd, size_t n)
{
	// printf("skip %u\n", n);
	if (!rcd->buf)
	{
		Pos += n;
		seek(esp, Pos);
	}
	rcd->pos += n;
}

//void make_top_grup_revisit

api grupp esp_top_grup(cespp esp, const char type[5])
{
	if (esp==NULL)
		return NULL;
	for (unsigned int i = 0; i < esp->grups->size; i++)
	{
	grupp grp = esp->grups->elements[i];
	if (*(unsigned int *)type == grp->hed->label)
	return esp->grups->elements[i];
	}
	return NULL;
}

inline struct form_id build_form_id(unsigned int formId)
{
	struct form_id form_id;
	form_id.formId = formId;
	form_id.modIndex = formId >> 24;
	form_id.objectIndex = formId & ~(form_id.modIndex << 24);
	return form_id;
}

// We do not map the TES4 -> MASTers
// For my mod, this is fine
api rcdp esp_get_form_id(unsigned int formId)
{
	struct form_id form_id;
	form_id = build_form_id(formId);
	espp esp = get_plugins()[form_id.modIndex];
	if (esp == NULL)
	return NULL;
	for (unsigned int j = 0; j < esp->records->size; j++)
	{
	rcdp rcd = esp->records->elements[j];
	if (rcd->form_id.objectIndex == form_id.objectIndex)
	return rcd;
	}
	return NULL;
}

/*
api void esp_array_loop(EspCArray *array, void (*func)(subrecord *field, void *data), void *data)
{
	for (int i = 0; i < array->size; i++)
	{
	func(array->elements[i], data);
	};
}
*/

api revised_array *esp_filter_objects(cespp esp, const char type[5])
{
	revised_array *filtered;
	narray(&filtered, 100);
	for (unsigned int i = 0; i < esp->records->size; i++)
	{
	record *record = esp->records->elements[i];
	if (record->hed->sgn == *(unsigned int *)type)
	insert(filtered, record);
	}
	return filtered;
}

char *uncompress_record(espp esp, rcdp rcd)
{
	//printf("uncompress\n");
	// char *src = rcd->data;
	// dont use rcd->data, use rcd->offset + sizeof header ?
	char *src = rcd->buf;
	unsigned int realSize = *(unsigned int *)src;
	unsigned int size = rcd->hed->size - 4;
	//printf("un size %i\n", size);
	src += 4;
	char *dest = malloc(realSize * sizeof(char));
	int ret = uncompress(dest, (uLongf*)&realSize, src, size);
	assertm(ret == Z_OK, "were zlib'd");
	rcd->size = realSize;
	if (!Buf) free(rcd->buf);
	rcd->buf = dest;
	return dest;
}

api esppp get_plugins()
{
	return plugins;
}

api espp has_plugin(const char *name)
{
	for (int i = PLUGINS; i-- > 0;)
	if (plugins[i] != NULL && 0 == strcmp(name, plugins[i]->filename))
	return plugins[i];
	return NULL;
}

api void free_plugin(esppp p)
{
	espp esp = *p;
	*p = NULL;
	if (esp == NULL)
	return;
	printf("free plugin\n");
	for (int i = PLUGINS; i --> 0; )
	if (esp == plugins[i])
	plugins[i] = NULL;
	for (unsigned int i = 0; i < esp->records->size; i++)
	{
	rcdp rcd = esp->records->elements[i];
	if (rcd->hed->flags & 0x00040000)
	free(rcd->buf);
	free_esp_array(&rcd->rcdbs);
	}
	free_esp_array(&esp->grups);
	free_esp_array(&esp->records);
	free(esp);
}

// Horrible growable c array stolen from GitHub

api void free_esp_array(revised_array ** array)
{
	//for (int i = (*array)->size; i --> 0; )
	//	free()
	free((*array)->elements);
}

inline void narray(revised_array ** p, unsigned int initial) {
	*p = malloc(sizeof(struct revised_array));
	revised_array * a = (*p);
	a->capacity = initial;
	a->size = 0;
	a->elements = calloc(a->capacity, sizeof(void *));
}

inline void grow(revised_array * a) {
	if (a->size != a->capacity)
	return;
	a->capacity *= 2;
	a->elements = realloc(a->elements, a->capacity * sizeof(void *));
}

inline void insert(revised_array * a, void * element) {
	if (a == NULL)
	return;
	grow(a);
	a->elements[a->size++] = element;
}

static int disk_read(espp esp, void **data, size_t size)
{
	if (size < 1)
		return 0;
	*data = malloc(size);
	int pos = fread(*data, size, 1, (FILE *)esp->stream);
	Pos += size;
	//assertm(pos == 1, "cant read 1");
	assertc(Pos == disk_tell(esp));
	return pos;
}

static int disk_seek(espp esp, long int offset)
{
	Pos = offset;
	int ok = fseek((FILE *)esp->stream, offset, SEEK_SET);
	return ok;
}

static long int disk_tell(espp esp)
{
	unsigned pos = ftell((FILE *)esp->stream);
	return pos;
}