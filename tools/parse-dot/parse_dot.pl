#!/usr/bin/env perl

use strict;
use warnings;

use Task;
use Data;
use Carp;

die "give a dot file" unless defined $ARGV[0] and -f $ARGV[0];


my %tasks;
my %data;
my @edges;
my @forks;

parse_file($ARGV[0], \%tasks, \%data, \@edges);
parse_edges(\%tasks, \%data, \@edges, \@forks);
compute_tasks_creators(\%tasks, \%data, \@forks);

#save data
my $file = $ARGV[0];
$file =~s/\.([^\.]+)$/.tasks/;
print (scalar keys(\%tasks));
print " tasks\n";
save($file, \%tasks);
$file =~s/\.([^\.]+)$/.data/;
print (scalar keys(\%data));
print " datas\n";
save($file, \%data);

sub compute_tasks_creators {
	my ($tasks, $data, $forks) = @_;
	for my $fork (@{$forks}) {
		my ($task_id, $data_id) = @{$fork};
		die "no data of id $data_id" unless exists $data->{$data_id};
		my $creator = $data->{$data_id}->get_producer();
		$tasks->{$task_id}->set_creator($creator);
	}
}

sub save {
	my $file = shift;
	my $hash = shift;
	my @elements = values %{$hash};
	#remove duplicated elements (because of init)
	my %h;
	for my $element (@elements) {
		$h{$element} = $element;
	}
	@elements = values %h;

	#now save
	open(FILE, "> $file") or die "unable to write $file";
	print FILE scalar @elements."\n";
	for my $e (@elements) {
		print FILE $e;
	}
	close(FILE);
}

sub parse_edges {
	my ($tasks, $data, $edges, $forks) = @_;

	for my $edge (@{$edges}) {
		my ($id1, $id2, $special) = @{$edge};
		my ($node_task_id, $node_data_id) = ($id1, $id2);
		my $direction = 'task to data';
		unless (exists $tasks->{$node_task_id}) {
			($node_task_id, $node_data_id) = ($id2, $id1);
			$direction = 'data to task';
		}
		if (exists $tasks->{$node_task_id} and exists $data->{$node_data_id}) {
			my $data_id = $data->{$node_data_id}->get_id();
			my $data_version = $data->{$node_data_id}->get_version();
			my $task_id = $tasks->{$node_task_id}->get_id();
			if ($direction eq 'task to data') {
				$data->{$node_data_id}->set_producer($task_id);
				$tasks->{$node_task_id}->add_output_data($data_id, $data_version);
			} else {
				$data->{$node_data_id}->add_depending_task($task_id);
				$tasks->{$node_task_id}->add_input_data($data_id, $data_version);
				if ($special) {
					push @{$forks}, [$node_task_id, $node_data_id];
				}
			}
		} else {
			#we maybe have two tasks
			#we discard this information
			unless ((exists $tasks->{$node_task_id}) and (exists $tasks->{$node_data_id})) {
				die "edge with non existing nodes $node_data_id ; $node_task_id";
			}
		}
	}
}

sub parse_file {

	my ($filename, $tasks, $data, $edges) = @_;
	open(FILE, "< $filename") or die "unable to open $file";

	#add a dummy start task
	$tasks->{'init'} = new Task("init", 0);
	$tasks->{'init'}->set_creator('');

	while(my $line = <FILE>) {
		chomp($line);
		if ($line =~ /^(\d+)\s+->\s+(\d+)/) {
			my ($id1, $id2) = ($1, $2);
			#we have an edge
			if ($line =~/arrowtail=diamond/) {
				#this information is important to find out which task creates which task
				push @{$edges}, [$id1, $id2, 1];
			} else {
				push @{$edges}, [$id1, $id2, 0];
			}
		} elsif($line =~ /^(\d+)\s+\[label=\"([^"]+)\", shape=(\S+),.*\];$/) {
			#we have a node
			my ($node_id, $label, $shape) = ($1, $2, $3);
			if ($shape eq 'ellipse') {
				#task
				if ($label =~/task=(0x[^\\]+).*date=(\d+)/) {
					my ($id, $duration) = ($1, $2);
					$tasks->{$node_id} = new Task($id, $duration);
				} else {
					die "unable to parse label $label for task node";
				}
			} elsif ($shape eq 'box') {
				#data
				if ($label =~/^(0x\S+)\s\((\d+)o\)\s+(\S+)$/) {
					my ($id, $size, $version) = ($1, $2, $3);
					$data->{$node_id} = new Data($id, $size, $version);
				} else {
					die "unable to parse label $label for data node";
				}
			} elsif ($shape eq 'diamond') {
				$tasks->{$node_id} = $tasks->{'init'};
			} else {
				die "unknown shape $shape";
			}
		} else {
			die "unknown line '$line'" unless $line eq 'digraph G {' or $line eq '}' or $line =~/^\s*$/;
		}
	}

	close(FILE);
}
