FuncFinder
==========

The FuncFinder command line application takes a regular expression and a list of
C++ source files and lists the entire function definition of any function that contains
the regular expression. It works similarly to <a href="https://en.wikipedia.org/wiki/Grep">
grep</a> with <code>-A NUM, --after-context=NUM</code> and <code>-B NUM, --before-context=NUM</code>
options except instead of a specified number of trailing and leading lines it prints
the entire function definition regardless of the number of lines.&nbsp; The 
output specifies the line with the regex as well as the range of lines of the 
function in the file.
    <pre class="code">&gt; FuncFinder
Usage: FuncFinder regex source_file ...
</pre>
    <p>
        It can be tedious to specify each source file individually, but under Unix-like
        environments like Linux or Mac OS X or even Windows with <a href="http://www.cygwin.com/">
        Cygwin</a> <code>find</code> and <code>xargs</code> can do that for you.
    </p>
    <pre class="code">&gt; find . -name "*.cpp" | xargs FuncFinder regex_search_string</pre>
    <p>
        Here is an example of running it on the <a href="http://www.theaceorb.com/">TAO</a>
        services source code looking for uses of <code>std::ifstream</code>.<br>
    </p>
    <pre class="code">&gt; cd ACE_wrappers-2.0/TAO/orbsvcs
&gt; find . -name "*.cpp" | xargs FuncFinder std::ifstream
== ./performance-tests/RTEvent/Colocated_Roundtrip/compare_histo.cpp(30) range [24,41]==


void
load_file (Vector &amp;vector,
           const char *filename)
{
  std::ifstream is (filename);
  if (!is)
    throw "Cannot open file";

  while (is &amp;&amp; !is.eof ())
    {
      long i; double v;
      is &gt;&gt; i &gt;&gt; v;
      Vector::value_type e (i, v);
      vector.insert (e);
    }
}
</pre>
    <p>
        Note the use of <code>std::ifstream</code> in the first line of the <code>load_file</code>
        function. Since FuncFinder found the search string in the compare_histo.cpp
        file, it reported the line number of <code>std::ifstream</code> and printed the
        entire function definition.
    </p>
    <p>
        FuncFinder is not written with performance particularly
        in mind, although performance is not completely ignored, it runs sufficiently quick for most use cases. You can speed up the
        previous example by incorporating the amazing performance of grep. The following
        produces the same output as above, but runs much faster.
    </p>
    <pre class="code">&gt; cd ACE_wrappers-2.0/TAO/orbsvcs
&gt; find . -name "*.cpp" | xargs grep -l std::ifstream | xargs FuncFinder std::ifstream
</pre>
