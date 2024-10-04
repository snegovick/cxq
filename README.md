# CXQ - the xq replacement

XQ is an awesome tool for xml querrying in bash scripts, but it is huge (~7 MB) which is not very nice for some embedded systems.

CXQ is based on libxml2 xpath example, and is tiny itself, but combined with libxml2 its size is about half of the size of XQ.

## Examples

Lookup apn nodes from apns db:

```
cxq apns-full-conf.xml /apns/apn[@mcc="260"][@mnc="06"]

```

Print values of attribute "user" from apns db:

```
cxq apns-full-conf.xml /apns/apn[@mcc="260"][@mnc="06"]/@user
```
