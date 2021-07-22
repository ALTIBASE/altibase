/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node /* these are stored in the table */
{  
    char *string;
    int length;
};

struct node table[] =
{    /* table to be searched */
    { "asparagus", 10 },
    { "beans", 6 },
    { "tomato", 7 },
    { "watermelon", 11 },
};

int node_compare(const void *node1, const void *node2)
{
    return (strcmp(
                   ((const struct node *)node1)->string,
                   ((const struct node *)node2)->string));


}

static int node_compare(const void *, const void *);

main()
{
    struct node *node_ptr, node;
    char str_space[20];   /* space to read string into */

    node.string = str_space;
    while (scanf("%20s", node.string) != EOF) {
        node_ptr = bsearch( &node,
                            table, sizeof(table)/sizeof(struct node),
                            sizeof(struct node), node_compare);
        if (node_ptr != NULL) {
            (void) printf("string = %20s, length = %d\n",
                          node_ptr->string, node_ptr->length);
        } else {
            (void)printf("not found: %20s\n", node.string);
        }
    }
    return(0);
}

