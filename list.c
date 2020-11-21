#include <stdio.h>
#include "list.h"

static int head_remained = LIST_MAX_NUM_HEADS; //number of available lists to build
static int node_remained = LIST_MAX_NUM_NODES; //number of available nodes
static List head_array[LIST_MAX_NUM_HEADS];
static Node node_array[LIST_MAX_NUM_NODES];
   
static int next_node_pos = 0;  //index of next free node in node_array
static int next_head_pos = 0;  //index of next free head in head_array

static int valve = 0; //only initialize two arrays at first time create a list

List* List_create(){
    //initialize head_array and node_array
    if(valve==0){
        int i;
        for(i=0;i<LIST_MAX_NUM_HEADS;i++){
            if(i!=LIST_MAX_NUM_HEADS-1){
                //with compund literal
                //pos,count,curr,first,last,next_available_head
                head_array[i] = (List){i,0,-1,-1,-1,i+1};
            }
            else{
                head_array[i] = (List){i,0,-1,-1,-1,-1};
            }   
        }

        int j;
        for(j=0;j<LIST_MAX_NUM_NODES;j++){
            if(j!=LIST_MAX_NUM_NODES-1){
                //item,prev,next,next_available_node
                node_array[j] = (Node){NULL,-1,-1,j+1};
            }
            else{
                node_array[j] = (Node){NULL,-1,-1,-1};              
            }
        }
        valve=1;
    }

    if(head_remained == 0) return NULL;
    int temp = next_head_pos; 
    head_remained--; //decrement number of head available 
    //update the index of next free had
    next_head_pos = head_array[temp].next_available_head; 
    return &(head_array[temp]);
}

int List_count(List* pList){
    return (pList->count);
}

void* List_first(List* pList){
    if(pList->first==-1) return NULL; //empty list
    pList->curr = pList->first;
    return (node_array[pList->first].item);
}

void* List_last(List* pList){
    if(pList->last==-1) return NULL;  //empty list
    pList->curr = pList->last;
    return (node_array[pList->last].item);
}

void* List_next(List* pList){
    //Empty list
    if(pList->count==0){
        return NULL;
    }
    //if curr is beyond the list or is at the last
    if(pList->curr==-2 || pList->curr == pList->last){
        pList->curr = -2;
        return NULL;
    }
    //if curr is before the list
    if(pList->curr==-1){
        pList->curr = pList->first;
        return (node_array[pList->curr].item);
    }
    pList->curr = node_array[pList->curr].next;
    return (node_array[pList->curr].item);

}

void* List_prev(List* pList){
    //Empty list
    if(pList->count==0){
        return NULL;
    }
    //if curr is before the list or is at the front
    if(pList->curr==-1 || pList->curr == pList->first){
        pList-> curr = -1;
        return NULL;
    }
    //if curr is beyond the list
    if(pList->curr==-2){
        pList->curr = pList->last;
        return (node_array[pList->curr].item);
    }
    pList->curr = node_array[pList->curr].prev;
    return (node_array[pList->curr].item);
}

void* List_curr(List* pList){
    if(pList->curr==-1 || pList->curr==-2){
        return NULL;
    }
    return (node_array[pList->curr].item);
}


int List_add(List* pList, void* pItem){
    if(node_remained==0) return -1;
    int temp = next_node_pos;
    next_node_pos = node_array[temp].next_available_node;
    node_remained--;
    node_array[temp].item = pItem;
    //empty list
    if(pList->count==0){
        pList->first = temp;
        pList->last = temp;
        pList->curr = temp;
        pList->count++;

    }
    else{
        //curr is before the list
        if(pList->curr==-1){
            node_array[pList->first].prev = temp;
            node_array[temp].next = pList->first;
            pList->first = temp;                  
        }
        //curr is at the end or beyond the list
        else if(pList->curr==-2 || pList->curr==pList->last){
            node_array[pList->last].next = temp;
            node_array[temp].prev = pList->last;
            pList->last = temp;        
        }
        else{
            int ori_next = node_array[pList->curr].next;
            node_array[pList->curr].next = temp;
            node_array[temp].prev = pList->curr;
            node_array[temp].next = ori_next;
            node_array[ori_next].prev = temp;
        }
        pList->curr = temp;
        pList->count++; 
    }
    return 0;
}

int List_insert(List* pList, void* pItem){
    if(node_remained==0) return -1;
    int temp = next_node_pos;
    next_node_pos = node_array[temp].next_available_node;
    node_remained--;
    node_array[temp].item = pItem;

    //empty list
    if(pList->count==0){
        pList->first = temp;
        pList->last = temp;
        pList->curr = temp;
        pList->count ++;
    }
    else{
        //curr is before the list or at the front
        if(pList->curr==-1 || pList->curr==pList->first){
            node_array[pList->first].prev = temp;
            node_array[temp].next = pList->first;
            pList->first = temp;
        }
        //curr is beyond the list
        else if(pList->curr==-2){
            node_array[pList->last].next = temp;
            node_array[temp].prev = pList->last;
            pList->last = temp;
        }
        else{
            int ori_prev = node_array[pList->curr].prev;
            node_array[pList->curr].prev = temp;
            node_array[temp].next = pList->curr;
            node_array[temp].prev = ori_prev;
            node_array[ori_prev].next = temp;
        }
        pList->count++;
        pList->curr = temp;
    }
    return 0;
}

int List_append(List* pList, void* pItem){
    if(node_remained==0) return -1;
    int temp = next_node_pos;
    next_node_pos = node_array[temp].next_available_node;
    node_remained--;

    node_array[temp].item = pItem;
    //empty list
    if(pList->count==0){
        pList->first = temp;
        pList->last = temp;

    }
    else{
        node_array[pList->last].next = temp;
        node_array[temp].prev = pList->last;
        pList->last = temp;
    }
    pList->curr = temp;
    pList->count++;
    return 0;
}

int List_prepend(List* pList, void* pItem){
    if(node_remained==0) return -1;
    int temp = next_node_pos;
    next_node_pos = node_array[temp].next_available_node;
    node_remained--;
    node_array[temp].item = pItem;
    
    if(pList->count==0){
        pList->first = temp;
        pList->last = temp;
    }

    else{
        node_array[pList->first].prev = temp;
        node_array[temp].next = pList->first;
        pList->first = temp;        
    }
    pList->curr = temp;
    pList->count++;
    return 0;
}



void* List_remove(List* pList){
    //empty list or curr is before or beyond the list
    if(pList->count == 0 || pList->curr==-1 || pList->curr==-2){
        return NULL;
    }

    else{      
        void *result = node_array[pList->curr].item;        
        int curr_temp = pList->curr;
        
        //only one node
        if(pList->count==1){
            pList->first=-1;
            pList->last=-1;
            pList->curr=-1;
        }
        //curr is the front
        else if(curr_temp == pList->first){
            pList->first = node_array[curr_temp].next;
            node_array[pList->first].prev = -1;
            pList->curr = pList->first;            
        }
        //curr is at the end
        else if(curr_temp == pList->last){
            pList->last = node_array[curr_temp].prev;
            node_array[pList->last].next=-1;
            pList->curr =-2;
        }
        else{
            int prev_temp = node_array[curr_temp].prev;
            int next_temp = node_array[curr_temp].next;
            node_array[prev_temp ].next = next_temp;
            node_array[next_temp].prev = prev_temp;
            pList->curr = next_temp;
        }
        pList->count--;
                
        int temp = next_node_pos;
        next_node_pos = curr_temp;
        node_array[curr_temp].next_available_node = temp;
        node_remained++;
        node_array[curr_temp].next = -1;
        node_array[curr_temp].prev = -1;
        node_array[curr_temp].item = NULL;
        return result;
    }
}


void List_concat(List* pList1, List* pList2){
    //both empty
    if(pList1->count==0 && pList2->count==0) return;
    //both not empty
    if(pList1->count>0 && pList2->count>0){
        node_array[pList1->last].next = pList2->first;
        node_array[pList2->first].prev = pList1->last;
        pList1->last = pList2->last;
        pList1->count = pList1->count + pList2->count;
    }
    //pList1 is empty
    else if(pList2->count>0){
        pList1->first = pList2->first;
        pList1->last = pList2->last;
        pList1->count = pList2->count;
    }
    int temp = next_head_pos;
    next_head_pos = pList2->pos;
    pList2->next_available_head = temp;
    head_remained++;
    pList2->curr = -1;
    pList2->last = -1;
    pList2->first = -1;
    pList2->count = 0;
}

void List_free(List* pList, FREE_FN pItemFreeFn){
    pList->curr = pList->first;
    int size = pList->count;
    while(size > 0){
        void* temp=List_remove(pList);
        pItemFreeFn(temp);
        size--;
    }

    pList->curr = -1;
    pList->last = -1;
    pList->first = -1;
    pList->count = 0;

    int temp = next_head_pos;
    next_head_pos = pList->pos;
    pList->next_available_head = temp;
    head_remained++;    
}

void* List_trim(List* pList){
    if(pList->count == 0){
        return NULL;
    }
    pList->curr = pList->last;
    void *result = List_remove(pList);
    if(pList->last!=-1){
        pList->curr = pList->last;
    }
    return result;
}

void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg){
    //empty list
    if(pList->count==0){
        pList->curr=-2;
        return NULL;
    }
    if(pList->curr==-2){
        return NULL;
    }
    if(pList->curr==-1){
        List_first(pList);
    }
    while(pList->curr!=-2){
        void* temp =List_curr(pList);
        if(pComparator(temp,pComparisonArg)){
            return temp;
        }
        List_next(pList);
    }
    return NULL;
}
