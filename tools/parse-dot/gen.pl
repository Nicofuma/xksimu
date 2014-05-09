#!/usr/bin/env perl

use strict;
use warnings;
no warnings "recursion";

use Task;
use Data;
use Carp;

my ($file, $levels, $min_degree, $max_degree, $min_time, $max_time, $min_size, $max_size) = @ARGV;

die "missing args" unless defined $max_size;

my %tasks;
my %data;

my $init = new Task('init', 0);
my $init_data = new Data('init', random_size(), 0);
$init_data->set_producer('init');
$init->add_output_data('init', 0);
my $end = new Task('end', 0);
my $n = 1;

$tasks{'init'} = $init;
$tasks{'end'} = $end;
$data{'init'} = $init_data;

fill_tasks(\%tasks, \%data, $init, $end, $levels-1);

#save data
$file =~s/\.([^\.]+)$/.tasks/;
print (scalar keys %tasks);
print " tasks\n";
save($file, \%tasks);
$file =~s/\.([^\.]+)$/.data/;
print (scalar keys %data);
print " datas\n";
save($file, \%data);

sub save {
	my $file = shift;
	my $elements = shift;
	my @elements = values %{$elements};
	#now save
	open(FILE, "> $file") or die "unable to write $file";
	print FILE scalar @elements."\n";
	for my $e (@elements) {
		print FILE $e;
	}
	close(FILE);
}

sub fill_tasks {
	my ($tasks, $data, $top, $bottom, $levels) = @_;
	if ($levels == 0) {
		for my $data_id (@{$top->{output_data}}) {
			$bottom->add_input_data($data_id, 0);
			$data{$data_id}->add_depending_task($bottom->get_id());
		}
		return;
	}
	my $nodes_number = int rand($max_degree - $min_degree) + $min_degree;
	for my $node_number (1..$nodes_number) {
		my $id = $n;
		$n = $n + 1;
		my $t_up = new Task("s_$id", random_duration());
		my $data2 = new Data("s_$id", random_size(), 0);
		$data2->set_producer("s_$id");
		$t_up->add_output_data("s_$id", 0);
		$t_up->add_input_data($top->get_id(), 0);
		my $t_down = new Task("e_$id", random_duration());

		$t_up->set_creator($top->get_id());
		$bottom->set_creator($t_down->get_id()) unless defined $bottom->{creator};

		$tasks{"s_$id"} = $t_up;
		$tasks{"e_$id"} = $t_down;

		my $data3 = new Data("e_$id", random_size(), 0);
		$data3->set_producer("e_$id");
		$t_down->add_output_data("e_$id", 0);
		$bottom->add_input_data("e_$id", 0);

		$data{"s_$id"} = $data2;
		$data{"e_$id"} = $data3;
			
		$data{$top->get_id()}->add_depending_task($t_up->get_id());
		$data{$t_down->get_id()}->add_depending_task($bottom->get_id());

		fill_tasks($tasks, $data, $t_up, $t_down, $levels-1);
	}
}

sub random_duration {
	return int rand ($max_time - $min_time) + $min_time;
}

sub random_size {
	return int rand ($max_size - $min_size) + $min_size;
}
