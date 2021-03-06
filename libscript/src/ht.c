
#include "ht.h"

#include <stdlib.h>
#include <string.h>

ht_item* ht_item_new(ht_key key, void* value) {
   ht_item* this;
   
   this = (ht_item*) calloc(sizeof(ht_item), 1);
   this->key = key;
   this->value = value;
   this->next = NULL;
   return this;
}

int ht_int_eq(ht* this, ht_key a, ht_key b) {
   return a.i == b.i;
}

int ht_string_eq(ht* this, ht_key a, ht_key b) {
   int lena = strlen(a.str);
   int lenb = strlen(b.str);
   if (lena != lenb)
      return 0;
   return (strncmp(a.str, b.str, lena) == 0);
}

int ht_int_hash(ht* this, ht_key key) {
   return (abs(key.i) % this->size);
}

int ht_string_hash(ht* this, ht_key key) {
   const char* str = key.str;
   unsigned long hash = 0;
   int c;

   /* http://www.cs.yorku.ca/~oz/hash.html */
   while ((c = *str++))
      hash = c + (hash << 6) + (hash << 16) - hash;

   return hash % this->size;
}

ht* ht_new(int size, ht_type type, int owner) {
   ht* this;
   
   this = (ht*) calloc(sizeof(ht), 1);
   this->size = size;
   this->buckets = (ht_item**) calloc(sizeof(ht_item*), size);
   this->type = type;
   switch (type) {
   case HT_STR:
      this->eq = ht_string_eq;
      this->hash = ht_string_hash;
      break;
   case HT_PTR:
   case HT_I:
      this->eq = ht_int_eq;
      this->hash = ht_int_hash;
   }
   this->owner = owner;
   return this;
}

void ht_delete(ht* this) {
   int i;
   for (i = 0; i < this->size; i++) {
      ht_item* walk = this->buckets[i];
      while (walk != NULL) {
         if (this->owner)
            free(walk->value);
         ht_item* save = walk;
         walk = save->next;
         free(save);
      }
   }
   free(this->buckets);
   free(this);
}

int ht_size(ht* this) {
   return this->items;
}

void ht_put(ht* this, ht_key key, void* value) {
   int index = this->hash(this, key);
   ht_item** bucket;
   for (bucket = &(this->buckets[index]); *bucket; bucket = &((*bucket)->next) ) {
      if (this->eq(this, (*bucket)->key, key)) {
         if (this->owner)
            free((*bucket)->value);
         (*bucket)->value = value;
         return;
      }
   }
   if (this->type == HT_STR) {
      char* copy = malloc(strlen(key.str) + 1);
      strcpy(copy, key.str);
      key.str = copy;
   }
   *bucket = ht_item_new(key, value);
   this->items++;
   return;
}

void* ht_take(ht* this, ht_key key) {
   int index = this->hash(this, key);
   ht_item** bucket; 
   
   for (bucket = &(this->buckets[index]); *bucket; bucket = &((*bucket)->next) ) {
      if (this->eq(this, (*bucket)->key, key)) {
         void* value = (*bucket)->value;
         ht_item* next = (*bucket)->next;
         if (this->type == HT_STR)
            free((*bucket)->key.str);
         free(*bucket);
         (*bucket) = next;
         this->items--;
         return value;
      }
   }
   return NULL;
}

void* ht_remove(ht* this, ht_key key) {
   void* value = ht_take(this, key);
   if (this->owner) {
      free(value);
      return NULL;
   }
   return value;
}

void* ht_get(ht* this, ht_key key) {
   int index = this->hash(this, key);
   ht_item* bucket;

   for(bucket = this->buckets[index]; bucket; bucket = bucket->next)
      if (this->eq(this, bucket->key, key))
         return bucket->value;
   return NULL;
}

void* ht_take_first(ht* this) {
   int i;
   for (i = 0; i < this->size; i++) {
      ht_item* bucket = this->buckets[i];
      if (bucket)
         return ht_take(this, bucket->key);
   }
   return NULL;
}

void ht_start(ht* this, ht_iterator* iter) {
   iter->table = this;
   iter->bucket = 0;
   iter->item = this->buckets[0];
}

void* ht_iterate(ht_iterator* iter) {
   void* result;
   for(;;) {
      if (!iter->item) {
         iter->bucket++;
         if (iter->bucket >= iter->table->size)
            return NULL;
         iter->item = iter->table->buckets[iter->bucket];
         continue;
      }
      result = iter->item->value;
      iter->item = iter->item->next;
      return result;
   }
}

