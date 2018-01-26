# MMN 13 - OS - Theory

Roman Smirnov 

## Q1

#### What are the pros and cons of a LIFO disk arm scheduling algorithm?

__Advantages__: Most recently requested data is retrieved first. Conceptually, there are cases where LIFO can have improved performance by making use of temporal and spatial locality. Example: a drive on a database server supporting multiple users - data queries made by same user at specific time are likely to be more local than queries made by different users.

__Disadvantages__: A request might never be retrieved if more keep coming. 

## Q2

#### Why is RAID2 interesting?

RAID2 is interesting because it performs fast, on-the-fly error checking and correction. It's also interesting because arrays with larger number of disks requires less proprotionally less Hamming code disks. So for very large disk arrays, overhead might be low enough to possibly make it worth looking into. 

## Q3

#### What's the required block count to hold the file in the systems?

__Parameters:__ 

- Block size: 0.5 Kbytes.
- Block address: 4 bytes.
- 12 I-node fields. 
- A single indirect block field.
- A double indirect block field. 
- A triple indirect block field.
- File size: 316 Kbytes.

__Answer__: 

The file is 316KB. Each block is 512B. Therefore, the number of required blocks to hold the data is 316*1024/512 = 632 blocks. Moreover, the number of Inodes needed to hold the addresses is ceil(632/12) = 53. These take ceil(53\*15\*4/512)=7 blocks to store. In total, we get 639 blocks including the inodes, we only need 2 extra blocks - for a total of 634 blocks. 

## Q4

#### How does revokation work in crypto protected C-lists?

Each user is given a capability to a file holding all the users capabilities. When a capability is revoked, it's deleted from the capability file. A capability is revoked by authenticating against the hash of concatenation of the target object, right, salt of a higher priviliged user and deleting the entry from the revokees capability file. 