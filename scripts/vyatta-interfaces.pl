#!/usr/bin/perl
#
# Module: vyatta-interfaces.pl
# 
# **** License ****
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# A copy of the GNU General Public License is available as
# `/usr/share/common-licenses/GPL' in the Debian GNU/Linux distribution
# or on the World Wide Web at `http://www.gnu.org/copyleft/gpl.html'.
# You can also obtain it by writing to the Free Software Foundation,
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
# MA 02110-1301, USA.
# 
# This code was originally developed by Vyatta, Inc.
# Portions created by Vyatta are Copyright (C) 2007 Vyatta, Inc.
# All Rights Reserved.
# 
# Author: Stig Thormodsrud
# Date: November 2007
# Description: Script to assign addresses to interfaces.
# 
# **** End License ****
#

use lib "/opt/vyatta/share/perl5/";
use Vyatta::Config;
use Vyatta::Misc;
use Vyatta::Interface;

use Getopt::Long;
use POSIX;
use NetAddr::IP;
use Fcntl;

use strict;
use warnings;

my $dhcp_daemon = '/sbin/dhclient';

my ($eth_update, $eth_delete, $addr, $dev, $mac, $mac_update, $op_dhclient);
my ($check_name, $show_names, $intf_cli_path, $vif_name);

GetOptions("eth-addr-update=s" => \$eth_update,
	   "eth-addr-delete=s" => \$eth_delete,
	   "valid-addr=s"      => \$addr,
           "dev=s"             => \$dev,
	   "valid-mac=s"       => \$mac,
	   "set-mac=s"	       => \$mac_update,
	   "op-command=s"      => \$op_dhclient,
	   "check=s"	       => \$check_name,
	   "show=s"	       => \$show_names,
	   "vif=s"	       => \$vif_name,
);

if ($eth_update)       { update_eth_addrs($eth_update, $dev); }
if ($eth_delete)       { delete_eth_addrs($eth_delete, $dev);  }
if ($addr)             { is_valid_addr($addr, $dev); }
if ($mac)	       { is_valid_mac($mac, $dev); }
if ($mac_update)       { update_mac($mac_update, $dev); }
if ($op_dhclient)      { op_dhcp_command($op_dhclient, $dev); }
if ($check_name)       { is_valid_name($check_name); }
if ($show_names)       { show_interfaces($show_names); }

sub is_ip_configured {
    my ($intf, $ip) = @_;
    my $wc = `ip addr show $intf | grep -c $ip`;
    if (defined $wc and $wc > 0) {
	return 1;
    } else {
	return 0;
    }
}

sub is_ip_duplicate {
    my ($intf, $ip) = @_;

    # 
    # get a list of all ipv4 and ipv6 addresses
    #
    my @ipaddrs = `ip addr show | grep inet | cut -d" " -f6`;
    chomp @ipaddrs;
    my %ipaddrs_hash = map { $_ => 1 } @ipaddrs;

    if (defined $ipaddrs_hash{$ip}) {
	#
	# allow dup if it's the same interface
	#
	if (is_ip_configured($intf, $ip)) {
	    return 0;
	}
	return 1;
    } else {
	return 0;
    }
}

sub touch {
    my $file = shift;
    my $t = time;

    sysopen (my $f, $file, O_RDWR|O_CREAT)
	or die "Can't touch $file: $!";
    close $f;
    utime $t, $t, $file;
}

sub dhcp_write_file {
    my ($file, $data) = @_;

    open(my $fh, '>', $file) || die "Couldn't open $file - $!";
    print $fh $data;
    close $fh;
}

sub dhcp_conf_header {
    my $output;

    my $date = `date`;
    chomp $date;
    $output  = "#\n# autogenerated by vyatta-interfaces.pl on $date\n#\n";
    return $output;
}

sub get_hostname {
    my $config = new Vyatta::Config;
    $config->setLevel("system");
    return $config->returnValue("host-name");
}

sub is_domain_name_set {
    my $config = new Vyatta::Config;
    $config->setLevel("system");
    return $config->returnValue("domain-name");
}

sub get_mtu {
    my $name = shift;
    my $intf = new Vyatta::Interface($name);
    return $intf->mtu();
}

sub dhcp_update_config {
    my ($conf_file, $intf) = @_;
    
    my $output = dhcp_conf_header();
    my $hostname = get_hostname();

    $output .= "interface \"$intf\" {\n";
    if (defined($hostname)) {
       $output .= "\tsend host-name \"$hostname\";\n";
    }
    $output .= "\trequest subnet-mask, broadcast-address, routers, domain-name-servers";
    my $domainname = is_domain_name_set();
    if (!defined($domainname)) {
       $output .= ", domain-name";
    } 

    my $mtu = get_mtu($intf);
    $output .= ", interface-mtu" unless $mtu;

    $output .= ";\n";
    $output .= "}\n\n";

    dhcp_write_file($conf_file, $output);
}

sub is_intf_disabled {
    my $name = shift;
    my $intf = new Vyatta::Interface($name);

    $intf or die "Unknown interface name/type: $name\n";

    # only do this if script run from config mode
    if (!defined $op_dhclient) {
	my $config = new Vyatta::Config;
	$config->setLevel($intf->path());

	return $config->exists("disable");
    }
    return 0;
}


sub run_dhclient {
    my $intf = shift;

    my ($intf_config_file, $intf_process_id_file, $intf_leases_file) = Vyatta::Misc::generate_dhclient_intf_files($intf);
    dhcp_update_config($intf_config_file, $intf);
    if (!(is_intf_disabled($intf))) {
      my $cmd = "$dhcp_daemon -q -nw -cf $intf_config_file -pf $intf_process_id_file  -lf $intf_leases_file $intf 2> /dev/null &";
      # adding & at the end to make the process into a daemon immediately
      system ($cmd) == 0
	or warn "start $dhcp_daemon failed: $?\n";
    }
}

sub stop_dhclient {
    my $intf = shift;
    if (!(is_intf_disabled($intf))) {
      my ($intf_config_file, $intf_process_id_file, $intf_leases_file) = Vyatta::Misc::generate_dhclient_intf_files($intf);
      my $release_cmd = "$dhcp_daemon -q -cf $intf_config_file -pf $intf_process_id_file -lf $intf_leases_file -r $intf 2> /dev/null";
      system ($release_cmd) == 0
	or warn "stop $dhcp_daemon failed: $?\n";
    }
}

sub update_eth_addrs {
    my ($addr, $intf) = @_;

    if ($addr eq "dhcp") {
	touch("/var/lib/dhcp3/$intf");
	run_dhclient($intf);
	return;
    } 
    my $version = is_ip_v4_or_v6($addr);
    if (!defined $version) {
	exit 1;
    }
    if (is_ip_configured($intf, $addr)) {
	#
	# treat this as informational, don't fail
	#
	print "Address $addr already configured on $intf\n";
	exit 0;
    }

    if ($version == 4) {
	return system("ip addr add $addr broadcast + dev $intf");
    }
    if ($version == 6) {
	return system("ip -6 addr add $addr dev $intf");
    }
    print "Error: Invalid address/prefix [$addr] for interface $intf\n";
    exit 1;
}

sub delete_eth_addrs {
    my ($addr, $intf) = @_;

    if ($addr eq "dhcp") {
	stop_dhclient($intf);
	unlink("/var/lib/dhcp3/dhclient_$intf\_lease");
	unlink("/var/lib/dhcp3/$intf");
	unlink("/var/run/vyatta/dhclient/dhclient_release_$intf");
        unlink("/var/lib/dhcp3/dhclient_$intf\.conf");
	exit 0;
    } 
    my $version = is_ip_v4_or_v6($addr);
    if ($version == 6) {
	    exec 'ip', '-6', 'addr', 'del', $addr, 'dev', $intf
		or die "Could not exec ip?";
    }

    ($version == 4) or die "Bad ip version";

    if (is_ip_configured($intf, $addr)) {
	# Link is up, so just delete address
	# Zebra is watching for netlink events and will handle it
	exec 'ip', 'addr', 'del', $addr, 'dev', $intf
	    or die "Could not exec ip?";
    }
	
    exit 0;
}

sub update_mac {
    my ($mac, $intf) = @_;

    open my $fh, "<", "/sys/class/net/$intf/flags"
	or die "Error: $intf is not a network device\n";

    my $flags = <$fh>;
    chomp $flags;
    close $fh or die "Error: can't read state\n";

    if (POSIX::strtoul($flags) & 1) {
	# NB: Perl 5 system return value is bass-ackwards
	system "sudo ip link set $intf down"
	    and die "Could not set $intf down ($!)\n";
	system "sudo ip link set $intf address $mac"
	    and die "Could not set $intf address ($!)\n";
	system "sudo ip link set $intf up"
	    and die "Could not set $intf up ($!)\n";
    } else {
	exec "sudo ip link set $intf address $mac";
    }
    exit 0;
}
 
sub is_valid_mac {
    my ($mac, $intf) = @_;
    my @octets = split /:/, $mac;
    
    ($#octets == 5) or die "Error: wrong number of octets: $#octets\n";

    (($octets[0] & 1) == 0) or die "Error: $mac is a multicast address\n";

    my $sum = 0;
    $sum += strtoul('0x' . $_) foreach @octets;
    ( $sum != 0 ) or die "Error: zero is not a valid address\n";

    exit 0;
}

sub is_valid_addr {
    my ($addr_net, $intf) = @_;

    if ($addr_net eq "dhcp") { 
	if ($intf eq "lo") {
	    print "Error: can't use dhcp client on loopback interface\n";
	    exit 1;
	}
	if (Vyatta::Misc::is_dhcp_enabled($intf)) {
	    print "Error: dhcp already configured for $intf\n";
	    exit 1;
	}
	if (is_address_enabled($intf)) {
	    print "Error: remove static addresses before enabling dhcp for $intf\n";
	    exit 1;
	}
	exit 0; 
    }

    my ($addr, $net);
    if ($addr_net =~ m/^([0-9a-fA-F\.\:]+)\/(\d+)$/) {
	$addr = $1;
	$net  = $2;
    } else {
	exit 1;
    }

    my $version = is_ip_v4_or_v6($addr_net);
    if (!defined $version) {
	exit 1;
    }

    my $ip = NetAddr::IP->new($addr_net);
    my $network = $ip->network();
    my $bcast   = $ip->broadcast();
    
    if ($ip->version == 4 and $ip->masklen() == 31) {
       #
       # RFC3021 allows for /31 to treat both address as host addresses
       #
    } elsif ($ip->masklen() != $ip->bits()) {
       #
       # allow /32 for ivp4 and /128 for ipv6
       #
       if ($ip->addr() eq $network->addr()) {
          print "Can not assign network address as the IP address\n";
          exit 1;
       }
       if ($ip->addr() eq $bcast->addr()) {
          print "Can not assign broadcast address as the IP address\n";
          exit 1;
       }
    }

    if (Vyatta::Misc::is_dhcp_enabled($intf)) {
	print "Error: remove dhcp before adding static addresses for $intf\n";
	exit 1;
    }
    if (is_ip_duplicate($intf, $addr_net)) {
	print "Error: duplicate address/prefix [$addr_net]\n";
	exit 1;
    }

    if ($version == 4) {
	if ($net > 0 && $net <= 32) {
	    exit 0;
	}
    } 
    if ($version == 6) {
	if ($net > 1 && $net <= 128) {
	    exit 0;
	}
    }

    exit 1;
}

sub op_dhcp_command {
    my ($op_command, $intf) = @_;

    if (!Vyatta::Misc::is_dhcp_enabled($intf)) {
        print "$intf is not using DHCP to get an IP address\n";
        exit 1;
    }
    
    my $flags = Vyatta::Misc::get_sysfs_value($intf, 'flags');
    my $hex_flags = hex($flags);
    if (!($hex_flags & 0x1)) {
     print "$intf is disabled. Unable to release/renew lease\n"; 
     exit 1;
    }

    my $tmp_dhclient_dir = '/var/run/vyatta/dhclient/';
    my $release_file = $tmp_dhclient_dir . 'dhclient_release_' . $intf;
    if ($op_command eq "dhcp-release") {
        if (-e $release_file) {
          print "IP address for $intf has already been released.\n";
          exit 1;
        } else {
          print "Releasing DHCP lease on $intf ...\n";
          stop_dhclient($intf);
	  mkdir ($tmp_dhclient_dir) if (! -d $tmp_dhclient_dir );
	  touch ($release_file);
          exit 0;
        }
    } elsif ($op_command eq "dhcp-renew") {
        print "Renewing DHCP lease on $intf ...\n";
        run_dhclient($intf);
	unlink ($release_file);
        exit 0;
    }

    exit 0;

}

sub is_valid_name {
    my $name = shift;
    my $intf = new Vyatta::Interface($name);

    exit 0 if $intf;

    die "$name: is not a known interface name\n";
}

# generate one line with all known interfaces (for allowed)
sub show_interfaces {
    my $type = shift;
    my @interfaces = getInterfaces();
    my @match;

    foreach my $name (@interfaces) {
	my $intf = new Vyatta::Interface($name);
	next unless $intf;		# skip unknown types
	next unless ($type eq 'all' || $type eq $intf->type());

	if ($vif_name) {
	    next unless $intf->vif();
	    push @match, $intf->vif()
		if ($vif_name eq $intf->physicalDevice());
	} else {
	    push @match, $name
		unless $intf->vif();
	}
    }
    print join(' ', @match), "\n";
}

exit 0;

# end of file
