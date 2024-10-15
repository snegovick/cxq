# CXQ - the xq replacement

XQ is an awesome tool for xml querrying in bash scripts, but it is huge (~7 MB) which is not very nice for some embedded systems.

CXQ is based on libxml2 xpath example, and is tiny itself. Combined with libxml2 the size of CXQ is about half of the size of XQ (depending on configuration of libxml2), most of which is occupied by the library.

## XML processing examples

Pretty-print XML file (at least libxml2 considers this pretty):

```
$ cxq -f test.xml
```

Simple lookup by attribute value:

```
$ cxq -f test.xml -x /test/test-node[@id="1"]
<test-node id="1" tag="first" name="Test name 1 first"/>
<test-node id="1" tag="second" name="Test name 1 second"/>
```

Lookup by two attributes:

```
$ cxq -f test.xml -x '/test/test-node[@id="1"][@tag="first"]'
<test-node id="1" nodetag="first" name="Test name 1 first"/>
```

Print attribute value:

```
$ cxq -f ./test.xml -x '/test/test-node[@id="1"][@tag="first"]'/@name
Test name 1 first
```

## Processing XML with namespaces
XML files and XPath queries with namespaces are also supported.

Find node by attribute value match:

```
$ cxq -f test-ns.xml -n "foo=http://foo.com/foo bar=http://bar.com/bar" -x '/test/test-node[@bar:id="1"]'
<test-node id="1" name="Test name"/>
```

Get attribute value:

```
$ cxq -f test-ns.xml -n "foo=http://foo.com/foo bar=http://bar.com/bar" -x '/test/test-node[@bar:id="1"]/@foo:name'
Test name
```

