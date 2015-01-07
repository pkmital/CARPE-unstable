/*********************************************************************
 * llist.cpp
 *
 * This file holds functions to create and manipulate linked lists
 * 
 * written by: Shawn Lankton (4/17/2009) - www.shawnlankton.com
 *
 ********************************************************************/

#include "llist.h"

// create a new point
PT *pt_create(long x, long y, long z){
  PT* newpt = (PT*)malloc(sizeof(PT));
  if(newpt == NULL) return NULL;

  newpt->x = x;
  newpt->y = y;
  newpt->z = z;
  newpt->prev = NULL;
  newpt->next = NULL;
  return newpt;
}

// create a new linked list
LL *ll_create(){
  LL *newll = (LL*)malloc(sizeof(LL));
  if(newll == NULL) return NULL;
  newll->head = NULL;
  newll->curr = NULL;
  newll->length = 0;
  return newll;
}

// push a point (add) onto the list (list)
void ll_push(LL *list, PT *add){
  if(add == NULL || list == NULL) return;
  add->next = list->head;
  add->prev = NULL;
  if(add->next != NULL){
    add->next->prev = add;
  }
  list->head = add;
  list->length++;
}

// push a new point (with data x,y,z) onto list (list)
void ll_pushnew(LL *list, long x, long y, long z){
  if(list == NULL) return;
  PT* add = pt_create(x,y,z);
  if(add == NULL) return;
  ll_push(list,add);
}

// free all points on a list (list) and the list itself
void ll_destroy(LL *list){
  if(list==NULL) return;
  while(list->head != NULL){
    ll_pop_free(list);
  }
  free(list);
}

// remove the "current" point on the list (list) and free the memory
void ll_remcurr_free(LL *list){
  PT* p = ll_remcurr(list);
  if(p != NULL) free(p);
}

// remove the "current" point on the list (list) and return a pointer to it
PT *ll_remcurr(LL *list){
  if(list == NULL) return NULL;
  PT* out = list->curr;
  if(out == list->head){
    return ll_pop(list);
  }
  else
  {
    if(out != NULL)
    {
      if(out->next != NULL){
        out->next->prev = out->prev;
      }
      if(out->prev != NULL){
        out->prev->next = out->next;
      }
      list->curr = out->next;
      list->length--;
    }
    return out;
  }
}

// remove the head of the list (list) and free it.
void ll_pop_free(LL *list){
  PT* p = ll_pop(list);
  if(p != NULL) free(p);
}

// remove the head of the list (list) and return it.
PT *ll_pop(LL *list){
  if(list == NULL) return NULL;
  PT *out = list->head;
  if(out != NULL){
    list->head = out->next;
    if(list->curr == out) list->curr = list->head;
    if(list->head != NULL) list->head->prev = NULL;
    list->length--;
  }
  return out;
}

// make the "current" point on a list the "head" of the list
void ll_init(LL *list){
  if(list == NULL) return;
  list->curr = list->head;
}

// advance the "current" point on a list to the "next" point in the list
void ll_step(LL *list){
  if(list == NULL) return;
  if(list->curr != NULL){
    list->curr = list->curr->next;
  }
}











