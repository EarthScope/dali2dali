# <p >dali2dali 
###  DataLink to DataLink</p>

1. [Name](#name)
1. [Synopsis](#synopsis)
1. [Description](#description)
1. [Options](#options)
1. [Caveats](#caveats)
1. [Author](#author)

## <a id='name'>Name</a>

<p >Copy selected data from one DataLink server to another</p>

## <a id='synopsis'>Synopsis</a>

<pre >
dali2dali [options] srchost desthost
</pre>

## <a id='description'>Description</a>

<p ><b>dali2dali</b> connects to one <u>DataLink</u> server, requests data streams and forwards the received packets to another <u>DataLink</u> server.</p>

<p >This program is designed to run continuously.  Because the DataLink protocol is stateful this program should be tolerant of connection breaks and subsequent re-connections.</p>

## <a id='options'>Options</a>

<b>-V</b>

<p style="padding-left: 30px;">Print program version and exit.</p>

<b>-h</b>

<p style="padding-left: 30px;">Print program usage and exit.</p>

<b>-v</b>

<p style="padding-left: 30px;">Be more verbose.  This flag can be used multiple times ("-v -v" or "-vv") for more verbosity.</p>

<b>-x </b><u>statefile</u>[:<u>interval</u>]

<p style="padding-left: 30px;">During client shutdown the last received packet ID and time stamp (start times) for each data stream will be saved in this file.  If this file exists upon startup the information will be used to resume the data streams from the point at which they were stopped.  In this way the client can be stopped and started without data loss, assuming the data are still available on the server.  If <u>interval</u> is specified the state will be saved every <u>interval</u> packets that are received.  Otherwise the state will be saved only on normal program termination.</p>

<b>-m </b><u>match</u>

<p style="padding-left: 30px;">Specify a matching expression to send to the server.  This regular expression is used to either limit the stream packets collected by matching against the stream ID, nominally in the form 'NET_STA_LOC_CHAN/TYPE'.  If the expression begins with an '@' character it is assumed to be a file containing a list of expressions for matching.</p>

<b>-r </b><u>reject</u>

<p style="padding-left: 30px;">Specify a rejecting expression to send to the server.  This regular expression is used to limit the stream packets collected and is logically opposite of the matching expression.  This expression is matched against the stream ID, nominally in the form 'NET_STA_LOC_CHAN/TYPE'.  If the expression begins with an '@' character it is assumed to be a file containing a list of expressions for rejecting.</p>

<b></b><u>srchost</u>

<p style="padding-left: 30px;">Specifies the address of the source DataLink server in host:port format. Either the host, port or both can be omitted.  If host is omitted then localhost is assumed, i.e.  ':16000' implies 'localhost:16000'.  If the port is omitted then 16000 is assumed, i.e.  'localhost' implies 'localhost:16000'.  If only ':' is specified 'localhost:16000' is assumed.</p>

<b></b><u>desthost</u>

<p style="padding-left: 30px;">Specifies the address of the destination DataLink server in host:port format (seed <i>srchost</i>).  The destination can be the same as the source with the effect, which has the potential of creating a runaway spiral of data duplication.</p>

## <a id='caveats'>Caveats</a>

<p >After receiving a data packet from the source server the program will forward the packet to the destination server.  If the connection to the destination server is broken the program will continuously try to re-connect, if the program is terminated before the record is forwarded the packet will be lost because the statefile will be written as if the record were sucessfully forwarded.  The potential, while quite small, can be minimized by running dali2dali on the same host as the destination server.</p>

## <a id='author'>Author</a>

<pre >
Chad Trabant
EarthScope Data Services
</pre>


(man page 2011/01/04)
