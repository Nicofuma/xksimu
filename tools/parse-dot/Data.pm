package Data;

use strict;
use warnings;
use Carp;
use overload
	'""' => \&stringify;

sub new {
	my $class = shift;
	my $self = {};
	$self->{id} = shift;
	$self->{size} = shift;
	$self->{version} = shift;
	confess "missing arg" unless defined $self->{version};
	$self->{depending_tasks_ids} = [];
        my $version = unpack "xA*", $self->{version};
        $self->{id} = $self->{id} . $version;
	bless $self, $class;
	return $self;
}

sub stringify {
	my $self = shift;
	my $tasks_count = @{$self->{depending_tasks_ids}};
	my $tasks_string = join("\n", @{$self->{depending_tasks_ids}});
	return "$self->{id}\n$self->{size}\n$self->{version}\n$self->{producer}\n$tasks_count\n$tasks_string\n";
}

sub add_depending_task {
	my $self = shift;
	my $task_id = shift;
	push @{$self->{depending_tasks_ids}}, $task_id;
}

sub set_producer {
	my $self = shift;
	my $task_id = shift;
	die "already a producer" if defined $self->{producer};
	$self->{producer} = $task_id;
}

sub get_producer {
	my $self = shift;
	return $self->{producer};
}

sub get_id {
	my $self = shift;
	return $self->{id};
}

sub get_version {
	my $self = shift;
	return $self->{version};
}

1;
