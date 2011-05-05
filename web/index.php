<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<?php

function show_page1()
{
	echo "

    <b>05/05/11</b> &nbsp;&nbsp;<b>git migration</b>
    <p>The mailtc source code repository has been migrated from subversion to git.<br />
    On a related note, i hope to get a new release out soon.</p><br /><br />

    <b>05/09/09</b> &nbsp;&nbsp;<b>mailtc 1.4.2 released</b>
    <p>After a long delay, a new release.  Everything has been rewritten entirely from scratch, with the aim of cleaning up and removing bloat.<br />
    The new code uses glib/GTK functions and classes wherever possible and now uses GnuTLS in place of OpenSSL, and also depends on libunique.<br />
    The IMAP plugins have been removed as they were untested and unmaintained, though it is still possible to write an external plugin for IMAP.<br />
    Future releases will concentrate on keeping things small and simple, and as portable as possible.</p><br /><br />

    <b>17/11/07</b> &nbsp;&nbsp;<b>mailtc 1.4.0 released</b>
    <p>This is a bugfix release to allow mailtc to build with GTK 2.12 and higher versions.<br />
    Systems with newer GTK versions will use the new GtkTooltips widget class, older versions will continue use the (now deprecated) GTkTooltip widget class.</p><br /><br />

    <b>24/09/07</b> &nbsp;&nbsp;<b>mailtc 1.3.9 released</b>
    <p>Time for another release, changes are as follows.<br />
    The plugin API was extended to allow custom plugin options via the config dialog.<br />
    All filter code was moved to the plugin code using the new plugin API.<br />
    All error reporting was improved and made more readable.</p><br /><br />

    <b>08/08/07</b> &nbsp;&nbsp;<b>mailtc 1.3.7 released</b>
    <p>The highlights of this release are as follows.<br />
    All configuration files (config/account/password/filter) have been moved to a single xml config file.  Mailtc now uses libxml2 for configuration.  NOTE the new configuration format requires that if you are using a previous release, you will need to run the migrate script found on the sourceforge project page, or re-enter your configuration details.<br />
    The filters were rewritten from scratch.  Now any number of filters can be added, and a number of extra mail header fields can be filtered.<br />
    A number of additional improvements and fixes were made, please see the ChangeLog for further information.</p><br /><br />

	<b>01/05/07</b> &nbsp;&nbsp;<b>mailtc 1.3.5 released</b>
    <p>I am very pleased to announce that mailtc 1.3.5 has been released!<br />
    mailtc now uses a message list for accounts, allowing greater mail information to be stored for future releases.<br />
    It is now possible to run a command (for example, play a sound) when a new mail arrives.<br />
    Network error reporting is now more configurable.<br />
    Many additional compile options available allowing users to build only what they require.<br />
    Cleaner envelope icons, and faster icon loading/unloading.<br />
    Vast tidy of code, with far greater error checking.<br />
    Various other improvements and bug fixes, please see the ChangeLog for full information.</p><br /><br />

    <b>28/01/07</b> &nbsp;&nbsp;<b>Update on upcoming mailtc release</b>
    <p>The SVN repository has been updated to reflect what will hopefully be the next mailtc release.<br />
    This source is still beta, and there is a still some testing to do, however it is almost ready for final release.  Until the release is available you can check out this source from SVN (see the download section for how to do this).<br />
    A summary of the main new features can be found in the TODO section on the web page.  Alternatively, for a more comprehensive list, please see the Changelog file in the current SVN source.</p><br /><br />

    <b>29/06/06</b> &nbsp;&nbsp;<b>Web site update</b>
	<p>A new 'Plugin' section has been added to the web site that provides some information on how to write a mailtc network plugin.</p><br /><br />

	<b>28/06/06</b> &nbsp;&nbsp;<b>mailtc 1.2.0 released</b>
	<p>This first post 1.0.0 release brings in a heap of new stuff:<br />
	As of this release, the code has been split into the core mailtc, and network plugins.  This means that the core now executes no network code.  It also greatly extends mailtc functionality, as it is now possible for any user to write a network plugin that can interface with mailtc.<br />
	All of the libgsasl code (for CRAM-MD5) was ported to OpenSSL.  mailtc now no longer requires libgsasl as a dependency, OpenSSL must now be compiled for CRAM-MD5 support.<br />
	mailtc now uses gettext and can now be internationalised (currently no translations are available).<br />
	A new compile option --enable-debug was added to compile with debugging enabled (for getting backtraces).<br />
	In network debug mode, the passwords are now hidden rather than written to stdout.<br />
	Most of the mailtc code was ported to use glib routines (the plugin code uses no glib/GTK code).<br />
	The application launching was much improved.<br />
	The error reporting was improved, and the date and time is now also output to the log whenever an error occurs.<br />
	Many other small bug fixes and improvements.</p><br /><br />

	<b>13/05/06</b> &nbsp;&nbsp;<b>mailtc 0.9.7 released</b>
	<p>mailtc now supports an unlimited number of accounts (previously the maximum was 6).<br />
	The code was made more efficient using linked lists, and the number of file reads reduced.<br />
	All strings are now located in a single file, making mailtc easier to localise.<br />
	A new 'multi' mode was added, that allows a different icon to be displayed if more than one account has new messages.<br />
	The mail application launching code was reworked so as to be less buggy.<br />
	General tidying up was made to the configuration dialog and error messages.<br />
	Finally, much testing and bug fixing with valgrind.</p><br /><br />

	<b>04/04/06</b> &nbsp;&nbsp;<b>Web site update</b>
	<p>A new 'Roadmap' section has been added to the web site to reflect the current status of the development work on mailtc.</p><br /><br />

	<b>21/02/06</b> &nbsp;&nbsp;<b>mailtc 0.8.0 released</b>
	<p>Rewrote the docklet code to make the icon loading and unloading nice and smooth.<br />
	Updated the tooltip code to display new mail information for all accounts.<br />
	A check is now made so that only one instance of mailtc can run at once.<br />
	An additional POP3 handler was added for mail headers that do not conform to the specification.<br />
	Many additional bugfixes added.</p><br /><br />

	<b>28/11/05</b> &nbsp;&nbsp;<b>mailtc 0.7.5 released</b>
	<p>The segfault problem when compiling with GCC 4 is now fixed.<br />
	A number of small warnings also found when compiling with GCC 4 were resolved.</p><br /><br />

	<b>19/11/05</b> &nbsp;&nbsp;<b>mailtc 0.7.3 released</b>
	<p>Fixed a few minor bugs and made a few minor improvements behind the scenes.<br />
	Made improvements to the error reporting.<br />
	Note: there are still problems when compiling with GCC 4, this will be fixed at some point in the future.<br />
	I also plan to make mailtc localised; if you would like to help (translate into another language) please contact me (dayul@users.sf.net).</p><br /><br />

	<b>11/09/05</b> &nbsp;&nbsp;<b>mailtc 0.7.0 released</b>
	<p>Implemented new mail filters option for POP3 and IMAP4.<br />
	Fixed a number of small bugs and made the code more portable.</p><br /><br />

	<b>24/07/05</b> &nbsp;&nbsp;<b>mailtc 0.6.3 released</b>
	<p>POP3 and IMAP over SSL are now supported (hint: you can now check gmail accounts).<br />
	Some memory leaks have been fixed after being found with Valgrind.<br />
	An custom account name can now be used for the new mail docklet message</p><br /><br />

	<b>06/07/05</b> &nbsp;&nbsp;<b>mailtc 0.5.6 released</b>
	<p>This release has much improved IMAP checking, with most of the IMAP code having been rewritten.<br />
	All reported bugs have also been fixed.</p><br /><br />

	<b>20/06/05</b> &nbsp;&nbsp;<b>mailtc 0.5.0 released</b>
	<p>This release brings CRAM-MD5 for IMAP along with a number of IMAP fixes.<br />
	A number of small bug fixes were also made including a problem with 0.4.4 being dependent on GTK 2.6.</p><br /><br />

	<b>12/06/05</b> &nbsp;&nbsp;<b>Web site update</b>
	<p>The mailtc web site has now been updated to use PHP rather than evil HTML frames.</p><br /><br />

	<b>05/06/05</b> &nbsp;&nbsp;<b>mailtc 0.4.4 released</b>
    <p>This release fixes a stack overflow error that crashed mailtc on some machines.<br />
	mailtc now has CRAM-MD5 authentication for POP3 (IMAP will follow soon)<br />
	The config dialog has been tidyed up and deprecated GTK functions have been removed (mailtc now requires GTK 2.4 or above).</p><br /><br />

	<b>25/05/05</b> &nbsp;&nbsp;<b>mailtc 0.4 released</b>
    <p>Its been a while and this release brings many new things.<br />
	mailtc now has basic IMAP support (this is very new so please report any errors you find)<br />
	A number of improvements involving the colour dialog and icon have been made<br />
	Cleaned up and improved a lot of code, and fixed compile problems on some systems<br />
	mailtc now checks mail instantly instead of waiting n minutes<br />
	OpenSSL is now optional (to compile without use --disable-ssl)<br />
	All mailtc processes can now be killed from the terminal using mailtc -k<br />
	mailtc has a new network debug mode (mailtc -d)</p><br /><br />

	<b>25/02/05</b> &nbsp;&nbsp;<b>mailtc 0.3.1 released</b>
	<p>A number of good features have been added.<br />
	mailtc now has APOP support!.<br />
	There is a new coloured icon format that lets you pick any icon colour, and they now display ok in fluxbox too.<br />
	There is now a tooltip for the icon that tells you the number of new messages.<br />
	I also sorted out a number of issues and cleaned a lot of code, making the mailtc binary half the size of the last release.</p><br /><br />

	<b>21/02/05</b> &nbsp;&nbsp;<b>mailtc 0.2.5 released</b>
	<p>This is mainly a cleanup release in preparation for IMAP.<br />
	Cleaned up a lot of code so it should use up less memory, and also added an icon size option so the icon is not truncated on fluxbox.</p><br /><br />

	<b>02/02/05</b> &nbsp;&nbsp;<b>mailtc 0.2 released</b>
	<p>This is the first mailtc release to support multiple mail accounts.</p><br /><br />
	";
}

function show_page2()
{
	echo "
	mailtc is a lightweight GTK2 mail checker for the system tray/notification area of panels.  It is designed primarily for *box window managers with lightweight panels such as fbpanel, but should work on any standards complient panel (such as GNOME/KDE/XFCE).<br /><br />
	<b>Features:</b><br />
	<ul>
	<li>POP3 checking.</li>
	<li>SSL/TLS authentication (requires GnuTLS).</li>
	<li>Envelope notification in system tray.</li>
	<li>Support for multiple mail accounts.</li>
	<li>Graphical configuration utility.</li>
	<li>Coloured envelope options.</li>
	<li>Option to launch mail program.</li>
	<li>Plugin architecture allowing additional network plugins to be used.</li>
	</ul><br /><br />
	<b>Requirements:</b><br />
	<ul>
	<li><a href=\"http://www.gtk.org\">Gtk 2.14 or above</a></li>
	<li><a href=\"http://live.gnome.org/LibUnique\">libunique</a></li>
	<li><a href=\"http://www.gnu.org/software/gnutls\">GnuTLS</a> (Optional but recommended)</li>
	<li>A standards compliant panel.</li>
	</ul><br /><br />
	<b>Contact:</b>
	<p>dayul@users.sf.net</p><br /><br />
	";
}

function show_page3()
{
	echo "
	<b>Next Release</b><br />
	<ul>
    <li>Add threading to allow clicking of icon while reading mail account. <span class=\"text_highlight\">(to do)</span></li>
    <li>GSocket for network routines. <span class=\"text_highlight\">(to do)</span></li>
	</ul><br /><br />
	";
}

function show_page4()
{
	echo "
	mailtc 1.3.5 on openbox showing new gmail message with new cleaner icon.<br />
    <p><a href=\"mailtc-1.3.5.png\"><img src=\"mailtc-1.3.5_thumb.png\" alt=\"mailtc 1.3.5 shot.png\" /></a></p><br /><br />
	mailtc 0.6 on openbox showing config dialog and gmail (POP3 SSL) account.<br />
    <p><a href=\"mailtc-0.6.png\"><img src=\"mailtc-0.6_thumb.png\" alt=\"mailtc 0.6 shot.png\" /></a></p><br /><br />
	mailtc 0.4 on openbox showing config dialog and network debug mode.<br />
    <p><a href=\"mailtc-0.4_conf.png\"><img src=\"mailtc-0.4_conf_thumb.png\" alt=\"mailtc 0.4 conf shot.png\" /></a>
    <a href=\"mailtc-0.4_debug.png\"><img src=\"mailtc-0.4_debug_thumb.png\" alt=\"mailtc 0.4 debug shot.png\" /></a></p><br /><br />
    mailtc 0.3 on fluxbox with new system tray icon and tooltip (screenshot by Ben Walton).<br />
    <p><a href=\"mailtc-0.3.png\"><img src=\"mailtc-0.3_thumb.png\" alt=\"mailtc 0.3 shot.png\" /></a></p><br /><br />
	";

}

function show_page5()
{
	echo "
	You can download the latest version from the project page <a href=\"http://sourceforge.net/projects/mailtc\">here</a>.<br /><br />

	You can also checkout the latest development source from the <a href=\"http://mailtc.git.sourceforge.net/git/gitweb.cgi?p=mailtc/mailtc;a=summary\">git repository here</a>, or alternatively use the command: <p><span class=\"text_highlight\">git clone git://mailtc.git.sourceforge.net/gitroot/mailtc/mailtc</span></p>
	";
}

function show_page6()
{

	echo "
	<b>Q. How do i install this mailtc thing?</b><br />
	<p><b>A.</b> You install mailtc by compiling it from source.
	First download the <a href=\"http://sourceforge.net/projects/mailtc\">latest package</a>, and extract it using the command
	<br /><br /><span class=\"text_highlight\">tar -jxvf mailtc-xx.tar.bz2</span><br /><br />
	In the mailtc directory that was extracted run the following commands:<br /><br />
	<span class=\"text_highlight\">./configure</span><br /><br />
	To compile without GnuTLS (this means that only the basic POP3 plugin will be installed) type:<br /><br />
	<span class=\"text_highlight\">./configure --disable-ssl</span><br /><br />
	Then type:<br /><br />
	<span class=\"text_highlight\">make<br />
	make install</span>.<br />
	<br />This will compile and install mailtc.<br /><br /><br /></p>
    <b>Q. So how do i use mailtc?</b><br />
	<p><b>A.</b> mailtc is a simple program,<br />
	Configure it by firstly running <span class=\"text_highlight\">mailtc -c</span>.<br />
	This will run the configuration dialog that allows you to add mail accounts to be checked.<br />
	Then run <span class=\"text_highlight\">mailtc &amp;</span> to start mailtc running in the background.<br />
	Test that mailtc is working by sending yourself an email, if mailtc is working, after a minute or two (or however long you set in the configuration dialog) an evelope icon should appear in the system tray.<br /><br />
	You can also put mailtc in your X startup script so that it runs automatically at startup.<br /><br />
	mailtc also has a network debug mode, to help diagnose problems if mailtc does not appear to be receiving mails<br />
	Type <span class=\"text_highlight\">mailtc -d</span> to run in network debug mode<br /><br />
	If you need to kill a running mailtc process, type <span class=\"text_highlight\">mailtc -k</span><br /><br /><br /></p>
	<!-- <b>Q. How do i write a network plugin for mailtc?</b><br />
	<p><b>A.</b> Please see the <a href=\"index.php?link=plugin\">plugin section</a> for more information on how to write a mailtc network plugin.<br /><br /><br /></p> -->
    <b>Q. Ive found a bug, how can i report it?</b><br />
	<p><b>A.</b> Please report all bugs on the <a href=\"http://sourceforge.net/projects/mailtc\">project page</a>, and ill try and fix them as soon as possible.<br /><br /><br /></p>
    <b>Q. I would like mailtc to ... can i have this feature?</b><br />
	<p><b>A.</b> Possibly.  Ask for it on the <a href=\"http://sourceforge.net/projects/mailtc\">project page</a><br /><br /></p>
    ";
}

function show_page7()
{
	echo "
	As of mailtc 1.2.0, all of the network code has been moved to separate plugins.<br />
	There were two reasons for this; firstly it is easier to manage this way, and secondly it now makes it possible for anyone to write their own network plugin for mailtc.<br /><br />
	There is currently no documentation on how to write a network plugin, however there is a dummy example plugin available.  This should be easy to understand (providing you know how to program in C), and can be used as a base to create your own network plugin.<br /><br />
	The example plugin can be downloaded from <a href=\"http://downloads.sourceforge.net/mailtc/mtc_dummyplug-1.3.9.tar.gz?download\">here</a>.  This plugin does not actually contain any network code, but demonstrates how to implement the relevant plugin routines for use with mailtc.
	";
}

?>

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />
    <link rel="stylesheet" href="styles.css" type="text/css" />
    <title>mailtc - <?php
        $linkvar = $_REQUEST["link"];
        print(ucfirst($linkvar? $linkvar: 'Home'));
        ?>
    </title>
</head>
<body>
    <div class="box">
		<a href="index.php?link=news">News</a><br />
		<a href="index.php?link=about">About</a><br />
		<a href="index.php?link=roadmap">Roadmap</a><br />
		<a href="index.php?link=screenshots">Screenshots</a><br />
		<a href="index.php?link=download">Download</a><br />
		<a href="index.php?link=faq">FAQ</a><br />
		<!-- <a href="index.php?link=plugin">Plugin</a><br /><br /> -->
		<a href="http://sourceforge.net">
		<img src="http://sourceforge.net/sflogo.php?group_id=129735&amp;type=1" class="sourceforge" alt="SourceForge.net Logo" /></a>
		<a href="http://sourceforge.net/donate/index.php?group_id=129735"><img src="http://sourceforge.net/images/project-support.jpg" class="sourceforge" alt="Support This Project" /></a>
	</div>
	<div class="content">
    <?php
        $linkvar = $_REQUEST["link"];
        if($linkvar)
        {
		    switch($linkvar)
		    {
			    //case plugin:      show_page7(); break;
			    case faq:         show_page6(); break;
			    case download:    show_page5(); break;
			    case screenshots: show_page4(); break;
			    case roadmap:     show_page3(); break;
			    case about:       show_page2(); break;
			    default:          show_page1();
            }
		}
        else
            show_page1();

        unset($linkvar);
	?>
    </div>
</body>
</html>

