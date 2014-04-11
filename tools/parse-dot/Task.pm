package Task;

use strict;
use warnings;

use overload
	'""' => \&stringify;
use Carp;

sub new {
	my $class = shift;
	my $self = {};
	$self->{id} = shift;
	$self->{duration} = shift;
	confess 'not enough arguments' unless defined $self->{duration};
	$self->{input_data} = []; #all data we need
	$self->{output_data} = []; #all data we produce
	$self->{created_tasks} = []; #array of all tasks which are created after our completion
	bless $self, $class;
	return $self;
}

sub stringify {
	my $self = shift;
	my $id_count = @{$self->{input_data}};
	my $id_string = join("\n", @{$self->{input_data}});
	my $od_count = @{$self->{output_data}};
	my $od_string = join("\n", @{$self->{output_data}});
	my $ct_count = @{$self->{created_tasks}};
	my $ct_string = join("\n", @{$self->{created_tasks}});
	confess "no creator for $self->{id}" unless defined $self->{creator};
	return "$self->{id}\n$self->{duration}\n$self->{creator}\n$id_count\n$id_string\n$od_count\n$od_string\n$ct_count\n$ct_string\n";
}

sub set_creator {
	my $self = shift;
	confess "already a creator for $self->{id}" if exists $self->{creator};
	$self->{creator} = shift;
}

sub add_created {
	my $self = shift;
	my $task_id = shift;
	push @{$self->{created_tasks}}, $task_id;
}

sub add_output_data {
	my $self = shift;
	my $data = shift;
	my $version = unpack "xA*", shift;
	push @{$self->{output_data}}, $data;
}

sub add_input_data {
	my $self = shift;
	my $data = shift;
	my $version = unpack "xA*", shift;
	push @{$self->{input_data}}, $data;
}

sub get_id {
	my $self = shift;
	return $self->{id};
}

1;
