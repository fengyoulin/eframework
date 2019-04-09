#include "hashtable.h"
#include <stdio.h>

int test_recap(){
	hashtable *ht = new_hash_table(15);
	if(ht->cap != 16){
		printf("16 != ht->cap=%d\n",ht->cap);
		return -1;
	}
    hash_free(ht);
    ht = new_hash_table(0);
    if(ht->cap != HASH_TABLE_INIT_CAP){
    	printf("%d != ht->cap=%d\n",HASH_TABLE_INIT_CAP,ht->cap);
    	return -1;
    }
    hash_free(ht);
    ht = new_hash_table(32);
    if(ht->cap != 32){
    	printf("32 != ht->cap=%d\n",ht->cap);
    	return -1;
    }
    
    hash_free(ht);
    ht = new_hash_table(33);
    if(ht->cap != 64){
    	printf("64 != ht->cap=%d\n",ht->cap);
    	return -1;
    }
    hash_free(ht);

    return 0;
}

int test_set(hashtable *ht){
    int res = hash_set(ht,"a","aaa");
    if(res < 0){
        printf("hash_set(a) fail\n");
        return res;
    }
    if(ht->used != 1){
        printf("ht->used:%d != 1\n",ht->used);
        return -1;
    }

    res = hash_set(ht,"ab","ababab");
    if(res < 0){
        printf("hash_set(ab) fail\n");
        return res;
    }
    if(ht->used != 2){
        printf("ht->used:%d != 2\n",ht->used);
        return -1;
    }

    res = hash_set(ht,"a","aaaaaaaaaa");
    if(res < 0){
        printf("hash_set(abc) fail\n");
        return res;
    }
    if(ht->used != 2){
        printf("ht->used:%d != 2\n",ht->used);
        return -1;
    }

    res = hash_set(ht,"ccccc","cccccc");
    if(res < 0){
        printf("hash_set fail %d\n",res);
        return res;
    }

    res = hash_set(ht,"ddddddd","ddddddd");
    if(res < 0){
        printf("hash_set fail %d\n",res);
        return res;
    }    

    return 0;
}

int test_get(hashtable *ht){
    bucket_val * v = hash_get(ht,"a");
    if(v == NULL){
        return -1;
    }
    if(!(v->type & HASH_VAL_TYPE_STR)){
        printf("type:%d != %d\n",v->type,HASH_VAL_TYPE_STR);
        return -1;
    }

    if(strcmp(v->v.str,"aaaaaaaaaa") != 0){
        printf("value:%s != %s\n",v->v.str,"aaaaaaaaaa");
        return -1;
    }

    return 0;
}

int test_remove(hashtable *ht){
    int res = hash_remove(ht,"a");
    if(res < 0){
        printf("hash_remove(a) fail\n");
        return res;
    }
    unsigned short e = hash_exists(ht,"a");
    if(e){
        printf("hash_exists(a)=%d\n",e);
        return -1;
    }
    return 0;
}

int test_resize(hashtable *ht){
    int i=1,res,origcap = ht->cap;
    res = hash_set(ht,"1111","11111");
    if(res < 0){
        printf("hash_set fail %d\n",i);
        return res;
    }

    res = hash_set(ht,"2222","22222");
    if(res < 0){
        printf("hash_set fail %d\n",i);
        return res;
    }

    res = hash_set(ht,"3333","33333");
    if(res < 0){
        printf("hash_set fail %d\n",i);
        return res;
    }

    res = hash_set(ht,"44444","44444");
    if(res < 0){
        printf("hash_set fail %d\n",i);
        return res;
    }

    res = hash_set(ht,"55555","55555");
    if(res < 0){
        printf("hash_set fail %d\n",i);
        return res;
    }

    if(ht->cap != 2*origcap){
        printf("ht->cap:%d != %d\n",ht->cap,2*origcap);
        return -1;
    }

    return 0;
}

int main(int argc,char* argv[]){	
    int res;
    res = test_recap();
    if(res == 0){
        printf("PASS hash_recap\n");
    }

    hashtable *ht = new_hash_table(0);
    res = test_set(ht);
    if(res == 0){
        printf("PASS hash_set\n");
    }

    res = test_get(ht);
    if(res == 0){
        printf("PASS hash_get\n");
    }
    res = test_remove(ht);
    if(res == 0){
        printf("PASS hash_remove\n");
    }
    res = test_resize(ht);
    if(res == 0){
        printf("PASS hash_resize\n");
    }
    hash_free(ht);
    return 0;
}