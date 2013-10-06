package ProFTPD::Tests::Modules::mod_fsquota;

use lib qw(t/lib);
use base qw(ProFTPD::TestSuite::Child);
use strict;

use File::Copy;
use File::Path qw(mkpath);
use File::Spec;
use IO::Handle;

use ProFTPD::TestSuite::FTP;
use ProFTPD::TestSuite::Utils qw(:auth :config :running :test :testsuite);

$| = 1;

my $order = 0;

my $TESTS = {
  fsquota_off_displayconnect => {
    order => ++$order,
    test_class => [qw(forking)],
  },

  fsquota_on_displayconnect => {
    order => ++$order,
    test_class => [qw(forking)],
  },

};

sub new {
  return shift()->SUPER::new(@_);
}

sub list_tests {
#  return testsuite_get_runnable_tests($TESTS);
  return qw(
    fsquota_off_displayconnect
  );
}

sub fsquota_off_displayconnect {
  my $self = shift;
  my $tmpdir = $self->{tmpdir};

  my $config_file = "$tmpdir/fsquota.conf";
  my $pid_file = File::Spec->rel2abs("$tmpdir/fsquota.pid");
  my $scoreboard_file = File::Spec->rel2abs("$tmpdir/fsquota.scoreboard");

  my $log_file = test_get_logfile();

  my $auth_user_file = File::Spec->rel2abs("$tmpdir/fsquota.passwd");
  my $auth_group_file = File::Spec->rel2abs("$tmpdir/fsquota.group");

  my $user = 'proftpd';
  my $passwd = 'test';
  my $group = 'ftpd';
  my $home_dir = File::Spec->rel2abs($tmpdir);
  my $uid = 500;
  my $gid = 500;

  # Make sure that, if we're running as root, that the home directory has
  # permissions/privs set for the account we create
  if ($< == 0) {
    unless (chmod(0755, $home_dir)) {
      die("Can't set perms on $home_dir to 0755: $!");
    }

    unless (chown($uid, $gid, $home_dir)) {
      die("Can't set owner of $home_dir to $uid/$gid: $!");
    }
  }

  auth_user_write($auth_user_file, $user, $passwd, $uid, $gid, $home_dir,
    '/bin/bash');
  auth_group_write($auth_group_file, $group, $gid, $user);

  my $connect_file = File::Spec->rel2abs("$tmpdir/connect.txt");
  if (open(my $fh, "> $connect_file")) {
    print $fh <<EOD;
            %{fsquota.user.enabled}
User quota: %{fsquota.user.enabled}
  Bytes: %{fsquota.user.kb.used} of %{fsquota.user.kb.total}
  Files: %{fsquota.user.files.used} of %{fsquota.user.files.total}
Group quota: %{fsquota.group.enabled}
  Bytes: %{fsquota.group.kb.used} of %{fsquota.group.kb.total}
  Files: %{fsquota.group.files.used} of %{fsquota.group.files.total}
EOD
    unless (close($fh)) {
      die("Can't write $connect_file: $!");
    }

  } else {
    die("Can't open $connect_file: $!");
  }

  my $config = {
    PidFile => $pid_file,
    ScoreboardFile => $scoreboard_file,
    SystemLog => $log_file,
    TraceLog => $log_file,
    Trace => 'DEFAULT:10 event:0 lock:0 scoreboard:0 signal:0 fsquota:20',

    AuthUserFile => $auth_user_file,
    AuthGroupFile => $auth_group_file,
    SocketBindTight => 'on',

    IfModules => {
      'mod_fsquota.c' => {
        FSQuotaEngine => 'off',

        DisplayConnect => $connect_file,
      },

      'mod_delay.c' => {
        DelayEngine => 'off',
      },
    },
  };

  my ($port, $config_user, $config_group) = config_write($config_file, $config);

  # Open pipes, for use between the parent and child processes.  Specifically,
  # the child will indicate when it's done with its test by writing a message
  # to the parent.
  my ($rfh, $wfh);
  unless (pipe($rfh, $wfh)) {
    die("Can't open pipe: $!");
  }

  my $ex;

  # Fork child
  $self->handle_sigchld();
  defined(my $pid = fork()) or die("Can't fork: $!");
  if ($pid) {
    eval {
      my $client = ProFTPD::TestSuite::FTP->new('127.0.0.1', $port);

      my $resp_code = $client->response_code();
      my $resp_msg = $client->response_msg();
      my $resp_msgs = $client->response_msgs();

      $client->quit();

      my $expected;

      $expected = 220;
      $self->assert($expected == $resp_code,
        test_msg("Expected response code $expected, got $resp_code"));

use Data::Dumper;
print STDERR "msgs:\n", Dumper($resp_msgs), "\n";

      $expected = 'Real Server';
      $self->assert(qr/$expected/, $resp_msg,
        test_msg("Expected response message '$expected', got '$resp_msg'"));
    };

    if ($@) {
      $ex = $@;
    }

    $wfh->print("done\n");
    $wfh->flush();

  } else {
    eval { server_wait($config_file, $rfh) };
    if ($@) {
      warn($@);
      exit 1;
    }

    exit 0;
  }

  # Stop server
  server_stop($pid_file);

  $self->assert_child_ok($pid);

  if ($ex) {
    test_append_logfile($log_file, $ex);
    unlink($log_file);

    die($ex);
  }

  unlink($log_file);
}

1;
