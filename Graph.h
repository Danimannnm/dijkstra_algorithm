#pragma once

#include "PriorityQueue.h"
#include "Queue.h"
#include <climits>
#include <vector>

// Uses adjacency list representation
struct Graph {
  struct Edge {
    size_t to;
    size_t weight;
  };

  std::vector<std::vector<Edge>> adj_list;

  Graph(const size_t num_vertices) : adj_list(num_vertices) {}

  void add_edge(size_t from, size_t to, size_t weight) {
    adj_list.at(from).push_back(Edge{to, weight});
  }

  void add_edge(size_t from, size_t to) { add_edge(from, to, 1); }

  void remove_edge(size_t from, size_t to) {
    auto &list = adj_list.at(from);
    for (size_t i = 0; i < list.size(); i++) {
      if (list.at(i).to == to) {
        list.erase(list.begin() + i);
        break;
      }
    }
  }

  void add_undirected_edge(size_t from, size_t to, size_t weight) {
    add_edge(from, to, weight);
    add_edge(to, from, weight);
  }

  void add_undirected_edge(size_t from, size_t to) {
    add_undirected_edge(from, to, 1);
  }

  void remove_undirected_edge(size_t from, size_t to) {
    remove_edge(from, to);
    remove_edge(to, from);
  }

  // Removes all edges from the specified vertex
  void remove_vertex(size_t vertex) {
    // Remove all edges to the vertex
    for (size_t i = 0; i < adj_list.size(); i++) {
      remove_edge(i, vertex);
    }

    // Remove all edges from the vertex
    adj_list.at(vertex).clear();
  }

  size_t num_vertices() const { return adj_list.size(); }

  size_t num_edges() const {
    size_t num_edges = 0;
    for (const auto &list : adj_list) {
      num_edges += list.size();
    }
    return num_edges;
  }

  std::vector<size_t> get_neighbors(size_t vertex) const {
    std::vector<size_t> neighbors;
    for (const auto &edge : adj_list.at(vertex)) {
      neighbors.push_back(edge.to);
    }
    return neighbors;
  }

  DoublyLinkedList<size_t> breadth_first_search(size_t start_vertex,
                                                size_t end_vertex) const {

    Queue<size_t> q;

    std::vector<bool> visited(num_vertices(), false);

    std::vector<size_t> parent(num_vertices(), SIZE_MAX);

    q.enqueue(start_vertex);
    visited.at(start_vertex) = true;

    while (!q.empty()) {
      auto vertex = q.dequeue();

      if (vertex == end_vertex) {
        break;
      }

      for (const auto &neighbor : get_neighbors(vertex)) {
        if (!visited.at(neighbor)) {
          q.enqueue(neighbor);
          visited.at(neighbor) = true;
          parent.at(neighbor) = vertex;
        }
      }
    }

    DoublyLinkedList<size_t> path{};
    if (!visited.at(end_vertex)) {
      return path;
    }

    size_t current_vertex = end_vertex;
    while (current_vertex != SIZE_MAX) {
      path.push_front(current_vertex);
      current_vertex = parent.at(current_vertex);
    }

    return path;
  }

  // Uses Djikstra's algorithm to find the shortest weighted path from the
  // start
  DoublyLinkedList<size_t> shortest_weighted_path(size_t start_vertex,
                                                  size_t end_vertex) const {
    PriorityQueue<size_t> queue;

    std::vector<size_t> distances(num_vertices(), SIZE_MAX);
    std::vector<size_t> previous(num_vertices(), SIZE_MAX);

    // Set the distance of the start vertex to 0
    distances.at(0) = 0;
    queue.enqueue(start_vertex, 0);

    while (!queue.empty()) {
      size_t vertex = queue.dequeue();
      if (vertex == end_vertex) {
        break;
      }

      // For each neighbor of the current vertex, update the distance if it is
      // shorter than the current distance
      for (const auto &edge : adj_list.at(vertex)) {
        size_t neighbor = edge.to;
        size_t distance = distances.at(vertex) + edge.weight;
        if (distance < distances.at(neighbor)) {
          distances.at(neighbor) = distance;
          previous.at(neighbor) = vertex;
          queue.enqueue(neighbor, distance);
        }
      }
    }

    DoublyLinkedList<size_t> path{};
    size_t current_vertex = end_vertex;
    if (previous.at(current_vertex) == SIZE_MAX) {
      return path;
    }

    while (current_vertex != SIZE_MAX) {
      path.push_front(current_vertex);
      current_vertex = previous.at(current_vertex);
    }
    return path;
  }

  // Checks if the specified vertex is part of any edge
  bool is_isolated(size_t vertex) const {
    for (const auto &list : adj_list) {
      for (const auto &edge : list) {
        if (edge.to == vertex) {
          return false;
        }
      }
    }
    return true;
  }
};