# Contributing to DNS Forwarding Server

Thank you for your interest in contributing to the DNS Forwarding Server project! This document provides guidelines and instructions for contributing to make the process smooth and effective.

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [How to Contribute](#how-to-contribute)
4. [Development Workflow](#development-workflow)
5. [Coding Standards](#coding-standards)
6. [Testing](#testing)
7. [Submitting Changes](#submitting-changes)
8. [Review Process](#review-process)
9. [Community](#community)

## Code of Conduct

This project adheres to a Code of Conduct that all participants are expected to follow. Please read and understand it before contributing.

In summary:
- Be respectful and inclusive
- Exercise consideration and empathy
- Focus on what is best for the community
- Give and gracefully accept constructive feedback

## Getting Started

### Prerequisites

To contribute to this project, you'll need:

- A C development environment
- Git for version control
- Make for building the project
- Basic understanding of DNS protocols (RFC 1035)

### Setting Up Your Development Environment

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/yourusername/dns-forwarding-server.git
   cd dns-forwarding-server
   ```
3. Add the original repository as an upstream remote:
   ```bash
   git remote add upstream https://github.com/originalowner/dns-forwarding-server.git
   ```
4. Build the project to verify your setup:
   ```bash
   make
   ```

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in the Issues section
2. If not, create a new issue with:
   - A clear, descriptive title
   - Detailed steps to reproduce the bug
   - Expected behavior and actual behavior
   - System information (OS, compiler version, etc.)
   - Any relevant logs or screenshots

### Suggesting Enhancements

1. Check existing issues for similar suggestions
2. Create a new issue with:
   - A clear title and detailed description
   - The rationale for the enhancement
   - Possible implementation approach
   - Any references or examples

### Documentation Improvements

Documentation improvements are always welcome! These include:

- Fixes to existing documentation
- New documentation sections
- Code examples and use cases
- Better explanations of features or design choices

## Development Workflow

We use a standard Git workflow:

1. Create a new branch for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```
   or
   ```bash
   git checkout -b fix/issue-you-are-fixing
   ```

2. Make your changes, following the coding standards

3. Keep your branch updated with upstream:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

4. Commit early and often with clear, descriptive messages

## Coding Standards

### C Style Guide

- Use 4 spaces for indentation, not tabs
- Maximum line length of 80 characters
- Function names should be in `snake_case`
- Variables should also use `snake_case`
- Constants and macros should be in `UPPER_CASE`
- Always use braces for control structures, even for single-line blocks
- Add comments for complex logic and function documentation

### Example

```c
/**
 * Gets the length of a domain name in a DNS packet.
 *
 * @param data Pointer to the start of the domain name
 * @return The length of the domain name including any compression pointers
 */
int get_name_length(const unsigned char *data) {
    const unsigned char *ptr = data;
    
    while (*ptr) {
        if ((*ptr & 0xC0) == 0xC0) {
            // Compressed name - 2 bytes
            return (ptr - data) + 2;
        }
        ptr += (*ptr + 1);
    }
    
    // Add one for null terminator
    return (ptr - data) + 1;
}
```

## Testing

Before submitting your changes, please ensure:

1. The code builds without warnings:
   ```bash
   make clean && make
   ```

2. Your changes work as expected:
   ```bash
   ./dns-server --resolver 8.8.8.8:53
   ```
   
3. You've tested both single and multi-question DNS queries

4. Memory management is correct (no leaks or invalid accesses)

## Submitting Changes

1. Push your branch to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

2. Create a Pull Request (PR) against the `main` branch of the original repository

3. In your PR description:
   - Reference any related issues
   - Describe what your changes do
   - Mention any breaking changes
   - Include testing you've performed

## Review Process

All submissions require review before being merged:

1. Maintainers will review your code for:
   - Correctness
   - Code quality
   - Adherence to project standards
   - Performance implications

2. Automated checks will test your code's buildability

3. You may be asked to make changes before your PR is accepted

4. Once approved, a maintainer will merge your changes

## Community

Join our community discussions:

- For quick questions: [GitHub Discussions](https://github.com/originalowner/dns-forwarding-server/discussions)
- For bug reports and feature requests: [GitHub Issues](https://github.com/originalowner/dns-forwarding-server/issues)

## Additional Resources

- [DNS Protocol (RFC 1035)](https://tools.ietf.org/html/rfc1035)
- [DNS Extensions (RFC 2671)](https://tools.ietf.org/html/rfc2671)
- [Socket Programming Guide](https://beej.us/guide/bgnet/)

Thank you for contributing to make DNS Forwarding Server better!